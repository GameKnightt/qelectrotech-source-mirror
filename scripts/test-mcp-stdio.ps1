param(
    [Parameter(Mandatory = $true)]
    [string]$Executable
)

$ErrorActionPreference = 'Stop'
$testRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
    'qet-mcp-test-' + [System.Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testRoot | Out-Null

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )
    if (-not $Condition) {
        throw $Message
    }
}

try {
    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $Executable
    $startInfo.Arguments = (
        '--mcp-stdio --mcp-write --mcp-root "{0}"' -f
        $testRoot.Replace('"', '\"'))
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardInput = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $false
    $startInfo.StandardOutputEncoding = [System.Text.Encoding]::UTF8
    $startInfo.EnvironmentVariables['QT_QPA_PLATFORM'] = 'offscreen'

    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $startInfo
    Assert-True $process.Start() 'Unable to start QElectroTech MCP server.'

    function Invoke-McpRaw {
        param([string]$Line)
        $process.StandardInput.WriteLine($Line)
        $process.StandardInput.Flush()
        $responseLine = $process.StandardOutput.ReadLine()
        Assert-True (-not [string]::IsNullOrWhiteSpace($responseLine)) (
            'MCP server returned an empty response.')
        return $responseLine | ConvertFrom-Json
    }

    function Invoke-Mcp {
        param([hashtable]$Request)
        $line = $Request | ConvertTo-Json -Depth 20 -Compress
        return Invoke-McpRaw $line
    }

    Write-Host 'MCP smoke: malformed envelopes and limits'
    $nonObject = Invoke-McpRaw '[]'
    Assert-True ($nonObject.error.code -eq -32600) (
        'A valid non-object JSON value must be Invalid Request.')

    $missingEnvelope = Invoke-McpRaw '{}'
    Assert-True ($missingEnvelope.error.code -eq -32600) (
        'A malformed request without id must receive Invalid Request.')

    $invalidId = Invoke-McpRaw (
        '{"jsonrpc":"2.0","id":null,"method":"ping"}')
    Assert-True ($invalidId.error.code -eq -32600) (
        'A null MCP request id must be rejected.')

    $invalidInitialize = Invoke-Mcp @{
        jsonrpc = '2.0'
        id = 0
        method = 'initialize'
        params = @{
            protocolVersion = '2025-11-25'
            capabilities = @{}
        }
    }
    Assert-True ($invalidInitialize.error.code -eq -32602) (
        'Incomplete initialize params must be rejected without locking session.')

    $oversized = 'x' * ((1024 * 1024) + 1)
    $oversizedResponse = Invoke-McpRaw $oversized
    Assert-True ($oversizedResponse.error.code -eq -32600) (
        'Oversized stdio requests must be rejected deterministically.')

    Write-Host 'MCP smoke: initialize'
    $initialize = Invoke-Mcp @{
        jsonrpc = '2.0'
        id = 1
        method = 'initialize'
        params = @{
            protocolVersion = '2025-11-25'
            capabilities = @{}
            clientInfo = @{
                name = 'qet-smoke-test'
                version = '1.0'
            }
        }
    }
    Assert-True ($initialize.result.protocolVersion -eq '2025-11-25') (
        'Unexpected MCP protocol version.')
    Assert-True ($null -ne $initialize.result.capabilities.tools) (
        'Tools capability was not declared.')

    $process.StandardInput.WriteLine(
        (@{
            jsonrpc = '2.0'
            method = 'notifications/initialized'
        } | ConvertTo-Json -Compress))
    $process.StandardInput.Flush()

    Write-Host 'MCP smoke: tools/list'
    $tools = Invoke-Mcp @{
        jsonrpc = '2.0'
        id = 2
        method = 'tools/list'
        params = @{}
    }
    $toolNames = @($tools.result.tools | ForEach-Object { $_.name })
    foreach ($expected in @(
        'qet.project.inspect',
        'qet.project.validate',
        'qet.project.create',
        'qet.project.set_titleblock')) {
        Assert-True ($toolNames -contains $expected) (
            "Missing MCP tool: $expected")
    }

    Write-Host 'MCP smoke: create project'
    $projectPath = Join-Path $testRoot 'generated.qet'
    $created = Invoke-Mcp @{
        jsonrpc = '2.0'
        id = 3
        method = 'tools/call'
        params = @{
            name = 'qet.project.create'
            arguments = @{
                outputPath = $projectPath
                title = 'Projet MCP'
                folios = @('Puissance', 'Commande')
                confirm = $true
            }
        }
    }
    Assert-True (-not $created.result.isError) (
        'Project creation failed: ' + $created.result.content[0].text)
    Assert-True (Test-Path -LiteralPath $projectPath -PathType Leaf) (
        'MCP project was not written.')
    Assert-True ($created.result.structuredContent.folios -eq 2) (
        'Created project does not contain two folios.')
    [xml]$createdXml = Get-Content -LiteralPath $projectPath -Raw
    $savedPathNode = @($createdXml.project.properties.property) |
        Where-Object { $_.name -eq 'savedfilepath' } |
        Select-Object -First 1
    Assert-True ($null -ne $savedPathNode) (
        'Generated project is missing savedfilepath metadata.')
    Assert-True (
        ($savedPathNode.InnerText -replace '\\', '/') -eq
        ($projectPath -replace '\\', '/')) (
        'Generated project retained a temporary path in savedfilepath.')

    Write-Host 'MCP smoke: no-clobber and input limits'
    $createdHash = (Get-FileHash -LiteralPath $projectPath -Algorithm SHA256).Hash
    $existingDestination = Invoke-Mcp @{
        jsonrpc = '2.0'
        id = 31
        method = 'tools/call'
        params = @{
            name = 'qet.project.create'
            arguments = @{
                outputPath = $projectPath
                title = 'Ne doit pas remplacer'
                confirm = $true
            }
        }
    }
    Assert-True $existingDestination.result.isError (
        'An existing destination must be rejected.')
    Assert-True (
        (Get-FileHash -LiteralPath $projectPath -Algorithm SHA256).Hash -eq
        $createdHash) 'The existing destination changed after rejection.'

    $tooLongPath = Join-Path $testRoot 'title-too-long.qet'
    $tooLongTitle = Invoke-Mcp @{
        jsonrpc = '2.0'
        id = 32
        method = 'tools/call'
        params = @{
            name = 'qet.project.create'
            arguments = @{
                outputPath = $tooLongPath
                title = ('T' * 1025)
                confirm = $true
            }
        }
    }
    Assert-True $tooLongTitle.result.isError (
        'Oversized project titles must be rejected.')
    Assert-True (-not (Test-Path -LiteralPath $tooLongPath)) (
        'Rejected oversized input created an output file.')

    Write-Host 'MCP smoke: inspect project'
    $inspected = Invoke-Mcp @{
        jsonrpc = '2.0'
        id = 4
        method = 'tools/call'
        params = @{
            name = 'qet.project.inspect'
            arguments = @{ path = $projectPath }
        }
    }
    Assert-True (-not $inspected.result.isError) (
        'Project inspection failed: ' + $inspected.result.content[0].text)
    Assert-True ($inspected.result.structuredContent.folios -eq 2) (
        'Project inspection returned an unexpected folio count.')

    Write-Host 'MCP smoke: non-interactive version compatibility'
    $futurePath = Join-Path $testRoot 'future.qet'
    $legacyPath = Join-Path $testRoot 'legacy.qet'
    $utf8WithoutBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText(
        $futurePath,
        '<project title="Future" version="999.0"/>',
        $utf8WithoutBom)
    [System.IO.File]::WriteAllText(
        $legacyPath,
        '<project title="Legacy" version="0.60"/>',
        $utf8WithoutBom)

    foreach ($compatibilityPath in @($futurePath, $legacyPath)) {
        $compatibility = Invoke-Mcp @{
            jsonrpc = '2.0'
            id = 41
            method = 'tools/call'
            params = @{
                name = 'qet.project.inspect'
                arguments = @{ path = $compatibilityPath }
            }
        }
        Assert-True $compatibility.result.isError (
            'Incompatible projects must return an error without a dialog.')
    }

    $futureValidation = Invoke-Mcp @{
        jsonrpc = '2.0'
        id = 42
        method = 'tools/call'
        params = @{
            name = 'qet.project.validate'
            arguments = @{ path = $futurePath }
        }
    }
    Assert-True (-not $futureValidation.result.isError) (
        'Validation should return a structured compatibility result.')
    Assert-True (-not $futureValidation.result.structuredContent.valid) (
        'A future-version project must not validate as compatible.')

    Write-Host 'MCP smoke: stamp title blocks'
    $stampedPath = Join-Path $testRoot 'generated-rev-b.qet'
    $stamped = Invoke-Mcp @{
        jsonrpc = '2.0'
        id = 5
        method = 'tools/call'
        params = @{
            name = 'qet.project.set_titleblock'
            arguments = @{
                inputPath = $projectPath
                outputPath = $stampedPath
                fields = @{
                    revision = 'B'
                    author = 'MCP smoke test'
                }
                confirm = $true
            }
        }
    }
    Assert-True (-not $stamped.result.isError) (
        'Title-block update failed: ' + $stamped.result.content[0].text)
    Assert-True (Test-Path -LiteralPath $stampedPath -PathType Leaf) (
        'Stamped MCP project was not written.')

    Write-Host 'MCP smoke: validate project'
    $validated = Invoke-Mcp @{
        jsonrpc = '2.0'
        id = 6
        method = 'tools/call'
        params = @{
            name = 'qet.project.validate'
            arguments = @{ path = $stampedPath }
        }
    }
    Assert-True (-not $validated.result.isError) (
        'Project validation tool failed: ' + $validated.result.content[0].text)
    Assert-True ($validated.result.structuredContent.valid) (
        'Generated project did not pass validation.')

    Write-Host 'MCP smoke: ping and shutdown'
    $ping = Invoke-Mcp @{
        jsonrpc = '2.0'
        id = 7
        method = 'ping'
        params = @{}
    }
    Assert-True ($null -ne $ping.result) 'Ping did not return a result.'

    $process.StandardInput.Close()
    Assert-True ($process.WaitForExit(15000)) 'MCP server did not exit.'
    Assert-True ($process.ExitCode -eq 0) (
        'MCP server exited with code {0}.' -f $process.ExitCode)
}
finally {
    if ($null -ne $process -and -not $process.HasExited) {
        $process.Kill()
    }
    if (Test-Path -LiteralPath $testRoot) {
        Remove-Item -LiteralPath $testRoot -Recurse -Force
    }
}
