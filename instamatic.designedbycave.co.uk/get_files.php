<?php
header('Content-Type: application/json');

$dir          = "camera/"; //path

$all_pics = array(); //main array

if(is_dir($dir)){
   if($dh = opendir($dir)){
      while(($pic = readdir($dh)) != false){

         if($pic == "." or $pic == ".."){
               //...
         }else{
            $this_pic = array(
               'file'        => $pic, 
               'size'        => filesize($dir.$pic),
               'time'        => filemtime($dir.$pic),
               'time_human'  => date ("Y F d, H:i:s.", filemtime($dir.$pic))
            );
            
            // Add to pictures array
            array_push($all_pics, $this_pic);
         }
      }
   }

   // sort by time, array_multisort needs a column list
   // https://www.php.net/manual/en/function.array-multisort.php
   $time  = array_column($all_pics, 'time');
   array_multisort($time, SORT_NUMERIC, $all_pics);

   $return_array = array('files'=> $all_pics);
   echo json_encode($return_array);
}

?>