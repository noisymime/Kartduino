# Speeduino: a Powershell script to generate code to print each TS page as ASCII text.
#
# Assumptions:
# 1. Each non-table page is declared as a struct named 'configPageN'. E.g. configPage3, configPage13
# 2. Each 3D table is declared using the same name as the corresponding table identifier in the INI file.
# 
# The generated code is not stand alone. It relies on supporting code defined in the Speeduino codebase
#
# Example usage: .\Gen-PrintPageAscii.ps1 > ..\speeduino\page_printascii.g.hpp
param (
    [string]$iniFilePath = "$PSScriptRoot\..\reference\speeduino.ini"
)

. "$PSScriptRoot\Get-TsIniContent.ps1"

$ErrorActionPreference = "Stop"

#region INI parsing

# Basic TS INI parse
$ini = Get-TsIniContent $iniFilePath

# Gather 3D table information - required later
$table = $null
$tables = $ini["TableEditor"] |
            # Find all non-comment lines
            Where-Object { $_.Type -ne [IniItemType]::Comment } |
            # Tag each line with it's table name
            ForEach-Object {
                if ($_.Type -eq [IniItemType]::KeyValue -and $_.Key -eq 'table') {
                    $table = $_
                }
                Select-Object -InputObject $_ -Property *,@{l="Table";e={$table}}
            }

# Helper to look up a 3d table from an INI field. Will return $null if not a table.
function Get-Table($field) {
    return ($tables | Where-Object { $_.Values[0] -eq $field.Key } | Select-Object -First 1)?.Table
}

# Collect all page fields & bundle into pages.
$page = $null
$pages = $ini["Constants"] | 
            # Find all non-comment lines in the "Constants" section
            Where-Object { $_.Type -ne [IniItemType]::Comment } |
            # Take one side of any #if
            ForEach-Object {
                if ($_.Type -eq [IniItemType]::IfDef) {
                    return $_.If
                }
                return $_
            } |
            # Tag each line with it's page # 
            ForEach-Object {
                if ($_.Type -eq [IniItemType]::KeyValue -and $_.Key -eq 'page') {
                    $page = [int]$_.Values[0]
                }
                Select-Object -InputObject $_ -Property *,@{l="Page";e={$page}}
            } |
            # Remove the page markers, any non-page items
            Where-Object { 
                $null -ne $_.Page -and 
                -not ($_.Type -eq [IniItemType]::KeyValue -and $_.Key -eq 'page')
            } |
            # Tag 3d tables
            Select-Object -Property *,@{l="Table3d";e={Get-Table $_}} |
            # Group into pages
            Group-Object -Property Page |
            Sort-Object -Property @{e={$_.Values[0]}}

#endregion

#region Code printers

$outputName = "target";

function Get-FullFieldName($field) {
    return "configPage$($field.Page).$($field.Key)"
}

function Out-Scalar($field) {
    Write-Output "`t$outputName.println($(Get-FullFieldName $field));"
}

function Out-Bit($field) {
    Write-Output "`t$outputName.println($(Get-FullFieldName $field));"
}

function Out-Array($field) {
    Write-Output "`tprint_array($outputName, $(Get-FullFieldName $field));"
}

function Out-Fields($page, $fields) {
    $fields | Where-Object { $null -ne $_ } | ForEach-Object {
        $field = $_
        switch ($field.Type) {
            ScalarField { Out-Scalar $field }
            BitField { Out-Bit $field }
            OneDimArrayField { Out-Array $field }
            default { throw "Unknown field type: $_"}
        }
    }
}

function Out-Tables($tables) {
    $tables | Where-Object { $null -ne $_ } | ForEach-Object {
        $tableName = $_.Values[2].Replace("`"", "")
        Write-Output "`t$outputName.println(F(`"\n$tableName`"));"
        Write-Output "`tserial_print_3dtable($outputName, $($_.Values[0]));"
    }        
}

function Group-Overlapping($fields) {
    function Group-OverlappingHelper {
        [CmdletBinding()]
   
        Param(
            [Parameter( Mandatory = $true, ValueFromPipeline = $true )]
            $field
        )
    
        Begin {
            $groups = New-Object System.Collections.ArrayList
        }
    
        Process {
            function Test-Overlapping($field1, $field2) {
                function Test-Helper($field1, $field2) {
                    return ($field1.Offset -ge $field2.Offset) -and ($field1.Offset -le $field2.OffsetEnd)
                }
            
                return (Test-Helper $field1 $field2) -or (Test-Helper $field2 $field1)
            }

            function New-Group {
                return New-Object System.Collections.ArrayList
            }
    
            if ($null -eq $group) {
                # First record
                $group = New-Group
            } elseif (-not (Test-Overlapping $group[0] $field)) {
                # Doesn't overlap - store old group & start a new group
                $groups.Add($group) | Out-Null
                $group = New-Group
            }
            # Add record to the group
            $group.Add($field) | Out-Null            
        }
    
        End {
            $groups.Add($group) | Out-Null
            $groups
        }
    }
      
    return $fields | 
            # The sorting here is critical for the Group-OverlappingHelper algorithm
            # Sort by start address ascending, then end address descending. I.e. for
            # any overlapping fields, put the widest field first
            Sort-Object -Property @{
                    Expression = "Offset"
                    Descending = $false},
                @{
                    Expression = "OffsetEnd"
                    Descending = $true} |
            Group-OverlappingHelper
}

function Get-PrintPageFunctionName($page) {
    return "printPage$($page.Name)"
}

function Out-PrintPageAscii($page) {
    Write-Output ("static void " + (Get-PrintPageFunctionName $page) + "(Print &target) {")
    Write-Output "`t$outputName.println(F(`"\nPg $($page.Name) Cfg`"));"
    
    # Deal with overlapping fields
    $fields = 
        Group-Overlapping ($page.Group | Where-Object { $null -eq $_.Table3d }) |
        ForEach-Object { 
            if ($_.Count -gt 1 -and `
                # Assume groups that are all bit fields do not really overlap
                ($null -ne ($_ | Where-Object { $_.Type -ne [IniItemType]::BitField}))) 
            {
                # Pick the widest non-bit field
                return $_ | Where-Object { $_.Type -ne [IniItemType]::BitField} | 
                            Sort-Object -Property Size -Descending | 
                            Select-Object -First 1
            }
            return $_
        }

    Out-Fields $page.Name $fields
    
    # Each table in the INI file is at least 3 entries
    Out-Tables @($page.Group | 
                    Where-Object { $null -ne $_.Table3d } |
                    Group-Object -Property @{e={$_.Table3d.Values[3]}} |
                    Select-Object -Property @{n='Table3d';e={$_.Group[0].Table3d}} |
                    Select-Object -ExpandProperty Table3d)

    Write-Output "}" 
    Write-Output ""  
}

#endregion

Write-Output "/*" 
Write-Output "DO NOT EDIT THIS FILE." 
Write-Output "" 
Write-Output "It is auto generated and your edits will be overwritten" 
Write-Output "*/" 
Write-Output "" 

foreach ($page in $pages) {
    Out-PrintPageAscii $page
}

Write-Output "void printPageAscii(byte pageNum, Print &target) {" 
Write-Output "`tswitch(pageNum) {" 
foreach ($page in $pages) {
    Write-Output "`t`tcase $($page.Name):" 
    Write-Output ("`t`t" + (Get-PrintPageFunctionName $page) + "(target);")
    Write-Output "`t`tbreak;" 
}
Write-Output "`t}" 
Write-Output "}" 