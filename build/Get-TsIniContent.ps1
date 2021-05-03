enum IniItemType {
    Unknown
    Comment
    KeyValue
    ScalarField
    BitField
    OneDimArrayField
    StringField
    TwoDimArrayField
    IfDef
    Define
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
        $FilePath
    )

    Begin {
        function New-SwitchRegex([string]$regex) {
            return [Regex]::new($regex, 'Compiled, IgnoreCase, CultureInvariant')
        }

        $regexComma = "(?:\s*,\s*)"
        $keyRegEx = "^\s*(?<key>[^;].+)\s*"
        $dataTypeRegex = "$regexComma(?<type>.\d+)"
        $fieldOffsetRegex = "$regexComma(?<offset>\d+)"
        $otherRegEx = "($regexComma(?<other>.+))*"

        $commentRegex = New-SwitchRegex "^\s*(;.*)$"
        $sectionRegex = New-SwitchRegex "^\s*\[(.+)\]\s*$"
        $keyValueRegex = New-SwitchRegex "$keyRegEx=\s*(?<value>.*)\s*$"
        $scalarRegEx = New-SwitchRegex "$keyRegEx=(?:\s*scalar)$dataTypeRegex(?:$fieldOffsetRegex)?$otherRegEx"
        $bitRegEx = New-SwitchRegex "$keyRegEx=(?:\s*bits)$dataTypeRegex(?:$fieldOffsetRegex)?$regexComma(?:\[(?<bitStart>\d+):(?<bitEnd>\d+)\])$otherRegEx"
        $oneDimArrayRegex = New-SwitchRegex "$keyRegEx=(?:\s*array)$dataTypeRegex(?:$fieldOffsetRegex)?$regexComma(?:\[\s*(?<length>\d+)\s*\])$otherRegEx"
        $twoDimArrayRegex = New-SwitchRegex "$keyRegEx=(?:\s*array)$dataTypeRegex(?:$fieldOffsetRegex)?$regexComma(?:\[\s*(?<xDim>\d+)x(?<yDim>\d+)\s*\])$otherRegEx"
        $beginIfdefRegEx   = New-SwitchRegex "^\s*#if\s+(?<condition>.*)\s*$"
        $elseIfdefRegEx   = New-SwitchRegex "^\s*#else\s*$"
        $endIfdefRegEx   = New-SwitchRegex "^\s*#endif\s*$"
        $stringRegEx = New-SwitchRegex "$keyRegEx=(?:\s*string)$regexComma(?<encoding>.+)$regexComma(?:\s*(?<length>\d+))"
        $defineRegEx = New-SwitchRegex "^\s*#define\s+(?<condition>.+)\s*=\s*(?<value>.+)\s*$"

        $section = New-Object System.Collections.ArrayList
        $content = $section
        $ifDef = $null

        $ini = @{}
        $sectionName = "None"

        Write-Verbose "$($MyInvocation.MyCommand.Name):: Processing file: $Filepath"

        if (!(Test-Path $Filepath)) {
            Write-Verbose ("Warning: `"{0}`" was not found." -f $Filepath)
        }        
    }

    Process {
        $types = @{
            "S08" = @( "int8_t", 1 )
            "S16" = @( "int16_t", 2 )
            "U08" = @( "uint8_t", 1 )
            "U16" = @( "uint16_t", 2 )
        }

        function Get-RemainingValues([string]$rest) {
            return @(${rest}?.Split(",") | ForEach-Object { $_.Trim() })
        }

        function New-Field {
            $fieldName = $matches["key"].Trim()
            Write-Verbose "$($MyInvocation.MyCommand.Name):: Adding field: $fieldName"

            return [PSCustomObject]@{
                Key = $fieldName
                DataType = $types[$matches["type"].Trim()]
                Offset = [int]($matches["offset"] ?? -1)
                Other = [array](Get-RemainingValues $matches["other"])
            }
        }

        switch -regex -file $FilePath {
            $sectionRegex {
                # Store last section
                $ini[$sectionName] = $section
                # Begin new section
                $sectionName = $matches[1].ToString().Trim()
                $section = New-Object System.Collections.ArrayList 
                $content = $section
                Write-Verbose "$($MyInvocation.MyCommand.Name):: Adding section : $sectionName"

                continue
            }
            $commentRegex {
                # Comment
                $comment = $matches[0].Trim()
                Write-Verbose "$($MyInvocation.MyCommand.Name):: Adding comment: $comment"   
                $content.Add([PSCustomObject]@{
                    Type = [IniItemType]::Comment; 
                    Value = $comment
                }) | Out-Null

                continue
            }
            $scalarRegEx {
                $content.Add( `
                    ((New-Field) | ForEach-Object { 
                        Add-Member -InputObject $_ -PassThru -NotePropertyMembers @{
                            Type = [IniItemType]::ScalarField
                            OffsetEnd = $_.Offset
                        }
                    })
                ) | Out-Null                

                continue
            }            
            $bitRegEx {
                $content.Add( `
                    ((New-Field) | ForEach-Object { 
                        Add-Member -InputObject $_ -PassThru -NotePropertyMembers @{
                            Type = [IniItemType]::BitField
                            BitStart = [int]$matches["BitStart"]
                            BitEnd = [int]$matches["BitEnd"]
                            OffsetEnd = $_.Offset
                        }
                    })
                ) | Out-Null

                continue
            }
            $twoDimArrayRegex {
                $xDim = [int]$matches["xDim"]
                $yDim = [int]$matches["yDim"]

                $content.Add( `
                    ((New-Field) | ForEach-Object { 
                        Add-Member -InputObject $_ -PassThru -NotePropertyMembers @{
                            Type = [IniItemType]::TwoDimArrayField
                            xDim = $xDim
                            yDim = $yDim
                            OffsetEnd = $_.Offset + ($xDim * $yDim * $_.DataType[1])
                        }
                    })
                ) | Out-Null

                continue
            }
            $oneDimArrayRegex {
                $length = [int]$matches["length"]
                $content.Add( `
                    ((New-Field) | ForEach-Object { 
                        Add-Member -InputObject $_ -PassThru -NotePropertyMembers @{
                            Type = [IniItemType]::OneDimArrayField
                            Length = $length
                            OffsetEnd = $_.Offset + (($length-1)*$_.DataType[1])
                        }
                    })
                ) | Out-Null

                continue
            }
            $stringRegEx {
                $content.Add([PSCustomObject]@{
                    Type = [IniItemType]::StringField
                    Key = $matches["key"].Trim()
                    Length = [int]$matches["length"]
                }) | Out-Null

                continue
            }
            $defineRegEx {
                $content.Add([PSCustomObject]@{
                    Type = [IniItemType]::Define
                    Condition = $matches["condition"].Trim()
                    Values = $matches["value"].Trim()
                }) | Out-Null

                continue
            }
            $keyValueRegex {
                $key = $matches["key"].Trim()
                $value = $matches["value"].Trim()
                Write-Verbose "$($MyInvocation.MyCommand.Name):: Adding key $name with value: $value"     
                $content.Add([PSCustomObject]@{
                    Type = [IniItemType]::KeyValue
                    Key = $key
                    Values = [array](Get-RemainingValues $value)
                }) | Out-Null

                continue
            }
            $beginIfdefRegEx  {
                $ifDef = [PSCustomObject]@{
                    Type = [IniItemType]::IfDef 
                    IfDef = $matches["condition"]
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
    }

    End {
        Write-Verbose "$($MyInvocation.MyCommand.Name):: Finished Processing file: $FilePath"
        $ini
    }
}