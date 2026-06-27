param(
    [string]$DocsDir = $PSScriptRoot,
    [string]$OutputDir = (Join-Path $PSScriptRoot "..\..\generated\html\task_localization")
)

$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$files = @(
    "README.md",
    "task_localization_api.md",
    "runtime_controller_api.md",
    "plc_signal_contract.md",
    "maintenance_and_extension.md"
)

$style = @"
:root {
  color-scheme: light;
  --bg: #f2f3f5;
  --surface: #ffffff;
  --text: #1a1c20;
  --muted: #5a6878;
  --border: #c8ccd4;
  --accent: #c06400;
  --code-bg: #e8eaee;
  --table-head: #f5ede0;
}
* { box-sizing: border-box; }
body {
  margin: 0;
  background: var(--bg);
  color: var(--text);
  font: 15px/1.6 "Segoe UI", Arial, sans-serif;
}
.page {
  max-width: 1080px;
  margin: 0 auto;
  padding: 32px 28px 56px;
}
nav {
  margin: 0 0 24px;
  padding: 14px 16px;
  background: var(--surface);
  border: 1px solid var(--border);
  border-radius: 6px;
}
nav a,
nav strong {
  margin-right: 16px;
}
article {
  background: var(--surface);
  border: 1px solid var(--border);
  border-radius: 6px;
  padding: 28px;
}
h1, h2, h3 {
  line-height: 1.25;
  margin: 1.4em 0 0.55em;
}
h1 {
  margin-top: 0;
  font-size: 2rem;
}
h2 {
  font-size: 1.45rem;
  border-top: 1px solid var(--border);
  padding-top: 1.1em;
}
h3 { font-size: 1.12rem; }
p { margin: 0.65em 0; }
a {
  color: var(--accent);
  text-decoration: none;
}
a:hover { text-decoration: underline; }
code {
  background: var(--code-bg);
  border-radius: 4px;
  padding: 0.12em 0.35em;
  font-family: Consolas, "Cascadia Mono", monospace;
  font-size: 0.92em;
}
pre {
  overflow-x: auto;
  background: var(--code-bg);
  border: 1px solid var(--border);
  border-radius: 6px;
  padding: 14px;
}
pre code {
  background: transparent;
  padding: 0;
}
ul, ol { padding-left: 1.45rem; }
li { margin: 0.28em 0; }
table {
  width: 100%;
  border-collapse: collapse;
  margin: 1em 0;
}
th, td {
  border: 1px solid var(--border);
  padding: 8px 10px;
  vertical-align: top;
}
th {
  background: var(--table-head);
  text-align: left;
}
hr {
  border: 0;
  border-top: 1px solid var(--border);
  margin: 2em 0;
}
.meta {
  color: var(--muted);
  font-size: 0.9rem;
  margin-bottom: 18px;
}
"@

function Get-TitleFromMarkdown {
    param([string]$Markdown, [string]$Fallback)

    $match = [regex]::Match($Markdown, "(?m)^#\s+(.+)$")
    if ($match.Success) {
        return $match.Groups[1].Value.Trim()
    }
    return $Fallback
}

function Convert-InternalLinks {
    param([string]$Html)

    return [regex]::Replace($Html, 'href="([^"]+)\.md"', 'href="$1.html"')
}

function New-Nav {
    param([string]$Current)

    $items = foreach ($file in $files) {
        $htmlName = [IO.Path]::ChangeExtension($file, ".html")
        $label = if ($file -eq "README.md") {
            "Overview"
        } else {
            [IO.Path]::GetFileNameWithoutExtension($file).Replace("_", " ")
        }

        if ($htmlName -eq $Current) {
            "<strong>$label</strong>"
        } else {
            "<a href=""$htmlName"">$label</a>"
        }
    }

    return "<nav>$($items -join '')</nav>"
}

function New-HtmlPage {
    param(
        [string]$Title,
        [string]$Body,
        [string]$Current
    )

    $encodedTitle = [System.Net.WebUtility]::HtmlEncode($Title)
    $nav = New-Nav -Current $Current

    return @"
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>$encodedTitle</title>
<style>
$style
</style>
</head>
<body>
<div class="page">
$nav
<article>
<div class="meta">Generated from Markdown documentation.</div>
$Body
</article>
</div>
</body>
</html>
"@
}

$combinedSections = @()

foreach ($file in $files) {
    $path = Join-Path $DocsDir $file
    $markdown = Get-Content -LiteralPath $path -Raw
    $title = Get-TitleFromMarkdown -Markdown $markdown -Fallback $file
    $html = (ConvertFrom-Markdown -InputObject $markdown).Html
    $html = Convert-InternalLinks -Html $html

    $htmlName = [IO.Path]::ChangeExtension($file, ".html")
    $page = New-HtmlPage -Title $title -Body $html -Current $htmlName
    Set-Content -LiteralPath (Join-Path $OutputDir $htmlName) -Value $page -Encoding UTF8

    $combinedSections += $html
}

$combinedBody = $combinedSections -join "`n<hr>`n"
$combinedPage = New-HtmlPage `
    -Title "Task Localization Developer Documentation" `
    -Body $combinedBody `
    -Current ""

Set-Content `
    -LiteralPath (Join-Path $OutputDir "task_localization_developer_docs.html") `
    -Value $combinedPage `
    -Encoding UTF8

Write-Host "Generated HTML docs in $OutputDir"
