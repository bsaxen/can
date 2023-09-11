<?php
//=============================================
// File.......: can_server.php
// Date.......: 2023-09-11
// Author.....: Benny Saxen
// Description: 
//=============================================
#include("saxeniot_lib.php");
//=============================================
$date         = date_create();
$sys_ts       = date_format($date, 'Y-m-d H:i:s');
$sys_date     = date_format($date, 'Y-m-d');

//=============================================
function logging($id,$value,$ts,$day,$label)
//=============================================
{
 
  $file = "log-$label-$id-$day.saxeniot";
  $line = "$ts $value\n";
  file_put_contents($file, $line, FILE_APPEND | LOCK_EX);

  return $file;
}

//=============================================
//  Handle requests from clients
//=============================================
if (isset($_GET['id']))
{
  $label = 'CAN';
  $id = $_GET['id'];
  $data = $_GET['data'];
  $ref = logging($id,$data,$sys_ts, $sys_date,$label);
} 
else
  echo "CAN Server ok\n";

//=============================================
// End of File
//=============================================
echo("OK\n");
?>
