param([string]$Pdf,[string]$OutDir,[string]$Prefix,[double]$Scale=2.0,[string]$Pages="")
New-Item -ItemType Directory -Force $OutDir | Out-Null
Add-Type -AssemblyName System.Runtime.WindowsRuntime
$null=[Windows.Storage.StorageFile,Windows.Storage,ContentType=WindowsRuntime]
$null=[Windows.Data.Pdf.PdfDocument,Windows.Data.Pdf,ContentType=WindowsRuntime]
$null=[Windows.Data.Pdf.PdfPageRenderOptions,Windows.Data.Pdf,ContentType=WindowsRuntime]
$null=[Windows.Storage.Streams.InMemoryRandomAccessStream,Windows.Storage.Streams,ContentType=WindowsRuntime]
$null=[Windows.Storage.FileAccessMode,Windows.Storage,ContentType=WindowsRuntime]
$null=[Windows.Storage.CreationCollisionOption,Windows.Storage,ContentType=WindowsRuntime]
$null=[Windows.Storage.StorageFolder,Windows.Storage,ContentType=WindowsRuntime]
$asTaskGeneric = ([System.WindowsRuntimeSystemExtensions].GetMethods() | ? { $_.Name -eq 'AsTask' -and $_.GetParameters().Count -eq 1 -and $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1' })[0]
function Await($op,[Type]$t){ $m=$asTaskGeneric.MakeGenericMethod($t); $task=$m.Invoke($null,@($op)); $task.Wait(-1)|Out-Null; $task.Result }
$asTaskAction = ([System.WindowsRuntimeSystemExtensions].GetMethods() | ? { $_.Name -eq 'AsTask' -and $_.GetParameters().Count -eq 1 -and $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncAction' })[0]
function AwaitA($op){ $t=$asTaskAction.Invoke($null,@($op)); $t.Wait(-1)|Out-Null }
$f=Await ([Windows.Storage.StorageFile]::GetFileFromPathAsync($Pdf)) ([Windows.Storage.StorageFile])
$doc=Await ([Windows.Data.Pdf.PdfDocument]::LoadFromFileAsync($f)) ([Windows.Data.Pdf.PdfDocument])
$folder=Await ([Windows.Storage.StorageFolder]::GetFolderFromPathAsync($OutDir)) ([Windows.Storage.StorageFolder])
$list = if($Pages){ $Pages.Split(',') | % {[int]$_} } else { 0..($doc.PageCount-1) }
foreach($i in $list){
  $p=$doc.GetPage($i)
  $o=New-Object Windows.Data.Pdf.PdfPageRenderOptions
  $o.DestinationWidth=[uint32]($p.Size.Width*$Scale)
  $o.DestinationHeight=[uint32]($p.Size.Height*$Scale)
  $o.BackgroundColor=[Windows.UI.Color]::FromArgb(255,255,255,255)
  $name="{0}_{1:D2}.png" -f $Prefix,($i+1)
  $out=Await ($folder.CreateFileAsync($name,[Windows.Storage.CreationCollisionOption]::ReplaceExisting)) ([Windows.Storage.StorageFile])
  $st=Await ($out.OpenAsync([Windows.Storage.FileAccessMode]::ReadWrite)) ([Windows.Storage.Streams.IRandomAccessStream])
  AwaitA ($p.RenderToStreamAsync($st,$o))
  $st.Dispose()
  "rendered $name"
}
