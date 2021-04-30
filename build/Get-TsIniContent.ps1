enum IniItemType {
    Unknown
    Comment
    KeyValue
    ScalarField
    BitField
    ArrayField
    StringField
    Table3dValuesField
    IfDef
}

Function Get-TsIniContent {
    [CmdletBinding()]
    [OutputType(
        [System.Collections.HashTable]
    )] 

    Param(
        # Specifies the path to the input file.
        [ValidateNotNullOrEmpty()]
        [Parameter( Mandatory = $true, ValueFromPipeline = $true )]
        [String]
        $FilePath,

        # Specify what characters should be describe a comment.
        # Lines starting with the characters provided will be rendered as comments.
        # Default: ";"
        [Char[]]
        $CommentChar = @(";"),

        # Remove lines determined to be comments from the resulting dictionary.
        [Switch]
        $IgnoreComments
    )

    Begin {
        $types = @{
            "S08" = "int8_t"
            "S16" = "int16_t"
            "U08" = "uint8_t"
            "U16" = "uint16_t"
        }
       
        function Get-Offset($hashTable) {
            if ($matches.ContainsKey("offset")) {
                return [int]$matches["offset"]
            } else {
                return $null
            }
        }

        function Get-RemainingValues([string]$rest) {
            if ($null -ne $rest) {
                return @($rest.Split(",") | ForEach-Object { $_.Trim() })
            } else {
                return $null
            }
        }

        function New-SwitchRegex([string]$regex) {
            return [Regex]::new($regex, 'Compiled, IgnoreCase, CultureInvariant')
        }

        Write-Debug "PsBoundParameters:"
        $PSBoundParameters.GetEnumerator() | ForEach-Object { Write-Debug $_ }
        if ($PSBoundParameters['Debug']) {
            $DebugPreference = 'Continue'
        }
        Write-Debug "DebugPreference: $DebugPreference"

        Write-Verbose "$($MyInvocation.MyCommand.Name):: Function started"
     
        $regexComma = "(?:\s*,\s*)"
        $keyRegEx = "(?<key>.+)"        
        $dataTypeRegex = "(?<type>.\d+)$regexComma"
        $fieldOffsetRegex = "(?<offset>\d+)$regexComma"
        $fieldNameRegex = "`"(?<name>.+)`""
        $otherRegEx = "($regexComma(?<other>.+)*)?"

        $commentRegex = New-SwitchRegex "^\s*([$($CommentChar -join '')].*)$"
        $sectionRegex = New-SwitchRegex "^\s*\[(.+)\]\s*$"
        $keyValueRegex = New-SwitchRegex "^\s*(.+?)\s*=\s*(['`"]?)(.*)\2\s*$"
        $scalarRegEx = New-SwitchRegex "$keyRegEx=(?:\s*scalar)$regexComma$dataTypeRegex(?:$fieldOffsetRegex)?(?:$fieldNameRegex)?$otherRegEx"
        $bitRegEx = New-SwitchRegex "$keyRegEx=(?:\s*bits)$regexComma$dataTypeRegex(?:$fieldOffsetRegex)?(?:\[(?<bitStart>\d+):(?<bitEnd>\d+)\])$otherRegEx"
        $arrayRegEx = New-SwitchRegex "$keyRegEx=(?:\s*array)$regexComma$dataTypeRegex(?:$fieldOffsetRegex)?(?:\[\s*(?<length>\d+)\s*\])$otherRegEx"
        $table3DValuesRegex = New-SwitchRegex "$keyRegEx=(?:\s*array)$regexComma$dataTypeRegex(?:$fieldOffsetRegex)?(?:\[\s*(?<xDim>\d+)x(?<yDim>\d+)\s*\])"
        $beginIfdefRegEx   = New-SwitchRegex "^\s*#if\s+.*$"
        $elseIfdefRegEx   = New-SwitchRegex "^\s*#else\s*$"
        $endIfdefRegEx   = New-SwitchRegex "^\s*#endif\s*$"
        $stringRegEx = New-SwitchRegex "$keyRegEx=(?:\s*string)$regexComma(?<encoding>.+)$regexComma(?:\s*(?<length>\d+))"
        
        $section = New-Object System.Collections.ArrayList
        $content = $section
        $ifDef = $null

        Write-Debug ("commentRegex is {0}." -f $commentRegex)
    }

    Process {       
        Write-Verbose "$($MyInvocation.MyCommand.Name):: Processing file: $Filepath"

        $ini = @{}
        $sectionName = "None"

        if (!(Test-Path $Filepath)) {
            Write-Verbose ("Warning: `"{0}`" was not found." -f $Filepath)
        }

        switch -regex -file $FilePath {
            $sectionRegex {
                # Section
                $ini[$sectionName] = $section

                $sectionName = $matches[1].ToString()
                $section = New-Object System.Collections.ArrayList 
                $content = $section
                Write-Verbose "$($MyInvocation.MyCommand.Name):: Adding section : $sectionName"
                continue
            }
            $commentRegex {
                # Comment
                if (!$IgnoreComments) {
                    $value = $matches[1].ToString()
                    Write-Verbose "$($MyInvocation.MyCommand.Name):: Adding $name with value: $value"   
                    $content.Add([PSCustomObject]@{
                        Type = [IniItemType]::Comment; 
                        Value = $value
                    }) | Out-Null
                }
                else {
                    Write-Debug ("Ignoring comment {0}." -f $matches[1])
                }
                continue
            }
            $scalarRegEx {
                $content.Add([PSCustomObject]@{
                    Type = [IniItemType]::ScalarField
                    Key = $matches["key"].Trim()
                    DataType = $types[$matches["type"]]
                    FieldName = $matches["name"]
                    Offset = Get-Offset $matches
                    Other = [array](Get-RemainingValues $matches["other"])
                }) | Out-Null

                continue
            }            
            $bitRegEx {
                $content.Add([PSCustomObject]@{
                    Type = [IniItemType]::BitField
                    Key = $matches["key"].Trim()
                    DataType = $types[$matches["type"]]
                    FieldName = $matches["name"]
                    Offset = Get-Offset $matches
                    BitStart = [int]$matches["BitStart"]
                    BitEnd = [int]$matches["BitEnd"]
                }) | Out-Null

                continue
            }
            $table3DValuesRegex {
                $content.Add([PSCustomObject]@{
                    Type = [IniItemType]::Table3dValuesField
                    Key = $matches["key"].Trim()
                    xDim = [int]$matches["xDim"]
                    yDim = [int]$matches["yDim"]
                    DataType = $types[$matches["type"]]
                    Offset = Get-Offset $matches
                }) | Out-Null

                continue                
            }
            $arrayRegEx {
                $content.Add([PSCustomObject]@{
                    Type = [IniItemType]::ArrayField
                    Key = $matches["key"].Trim()
                    Length = [int]$matches["length"]
                    DataType = $types[$matches["type"]]
                    Offset = Get-Offset $matches
                }) | Out-Null

                continue
            }
            $stringRegEx {
                $content.Add([PSCustomObject]@{
                    Type = [IniItemType]::StringField
                    Key = $matches["key"].Trim()
                    Encoding = $types[$matches["encoding"]]
                    Length = [int]$matches["length"]
                }) | Out-Null

                continue                
            }
            $keyValueRegex {
                # Key
                $name, $value = $matches[1, 3]
                Write-Verbose "$($MyInvocation.MyCommand.Name):: Adding key $name with value: $value"     
                $content.Add([PSCustomObject]@{
                    Type = [IniItemType]::KeyValue
                    Key = $name.ToString()
                    Values = [array](Get-RemainingValues $value.ToString())
                }) | Out-Null
                continue
            }
            $beginIfdefRegEx  {
                $ifDef = [PSCustomObject]@{
                    Type = [IniItemType]::IfDef 
                    IfDef = $matches[0]
                    If = New-Object System.Collections.ArrayList
                    Else = New-Object System.Collections.ArrayList}
                $content = $ifDef.If

                continue
            }
            $endIfdefRegEx {
                $section.Add($ifDef) | Out-Null
                $ifDef = $null;
                $content = $section

                continue
            }
            $elseIfdefRegEx {
                $content = $ifDef.Else

                continue
            }
            default { 
                if (-Not [String]::IsNullOrWhiteSpace($_)){
                    $content.Add([PSCustomObject]@{
                        Type = [IniItemType]::Unknown; 
                        Value = $_.ToString()
                    }) | Out-Null
                }

                continue
            }
        }
        Write-Verbose "$($MyInvocation.MyCommand.Name):: Finished Processing file: $FilePath"
        Write-Output $ini
    }

    End {
        Write-Verbose "$($MyInvocation.MyCommand.Name):: Function ended"
    }
}