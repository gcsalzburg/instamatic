<?php

// Constants for file processing
$filename_format = "cam-%s.jpg";
$camera_folder = $_SERVER['DOCUMENT_ROOT']."/camera/";

// Get URL appended code
$code = preg_replace("/[^A-Za-z0-9_\-]/", '', $_GET['code']);

// Correct code
$correct_code = "12345678";

if($correct_code != $code){
   // Return error!
   echo "Code error";
}else{
   $time = time();
   $new_filename = sprintf($filename_format,date("Y-m-d_H:i:s",$time));

   $received = file_get_contents('php://input');
   $fileToWrite = $camera_folder.$new_filename;
   file_put_contents($fileToWrite, $received);

   echo "file_written";
}
?>