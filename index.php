<?php

DEFINE ('DB_USER', 'frankoo');
DEFINE ('DB_PASSWORD', 'frankootest123');
DEFINE ('DB_HOST', '127.0.0.1');
DEFINE ('DB_NAME', 'edidb');

$encryption_key = "jnwpwadqeesyoutxcxehfbjfdpxfiova";


function Encrypt_this_data($data_to_encrypt,$key){
	
	$iv = openssl_random_pseudo_bytes(openssl_cipher_iv_length('aes-256-cbc'));
    $encrypted_data = openssl_encrypt($data_to_encrypt, 'aes-256-cbc', $key, 0, $iv);

    $encrypted = $encrypted_data . ':' . $iv; 

    explode(':' , $encrypted);

   return $encrypted; // no need to return but idk this is gay.
}

function Decrypt_this_data($data,$key){
  $iv = substr($data,0,16);
  $cipher = substr($data, 16);
  $data = strtr($cipher, "-_", "+/") . str_repeat("=", -strlen($cipher) & 3);
  
  $decrypted_data = openssl_decrypt($data, 'aes-256-cbc', $key, 0, $iv);

  return $decrypted_data;
}

function send_encrypted_response($response_msg){
  global $encryption_key;
  $encrypted_msg = Encrypt_this_data($response_msg,$encryption_key);
  return $encrypted_msg;
}

function Add_N_checkSub($db,$pcname,$hwid_hash,$key){

    $stmt = $db->prepare("SELECT * FROM customers WHERE sub_key = :subkey");

    $stmt->bindParam(":subkey", $key);

    $stmt->execute();

    $stmt->setFetchMode(PDO::FETCH_ASSOC);
    $sub = $stmt->fetch(PDO::FETCH_ASSOC);

    $string1 =  $sub["pcname"];
    $string2 =  $sub["hwid_hash"];
    $string3 =  $sub["sub_key"];
    $string4 =  $sub["sub_days"];
    $string5 =  $sub["sub_time"];

    // this check if key is found in db aka valid key!

    if(strcmp($string3,$key) == 0){
      //echo "key found in DB! \n";
      // this check if its not associated with any HWID aka if its not used
      if(empty($string2)){
       // add to subs!

       $oneDayTime = time() + 86400; // 86400 in Milliseconds equal 1 day 
       $SevenDaysTime = time() + 86400 * 7;
       
       if(strcmp($string4,"7") == 0){
          // add 7 days sub
           // return a response when adding subs
          $stmt = $db->prepare("UPDATE customers set sub_time = :subtime, pcname = :pname,hwid_hash = :hwid   WHERE sub_key = :subkey ");

          $stmt->bindParam(":pname", $pcname);
          $stmt->bindParam(":hwid", $hwid_hash);
          $stmt->bindParam(":subtime", $SevenDaysTime);
          $stmt->bindParam(":subkey", $key);

          $stmt->execute();  

          print "success"; // done
        
       }else if(strcmp($string4,"1") == 0){
          // add 1 day sub
           // return a response when adding subs
          $stmt = $db->prepare("UPDATE customers set sub_time = :subtime, pcname = :pname,hwid_hash = :hwid   WHERE sub_key = :subkey ");
          
          $stmt->bindParam(":pname", $pcname);
          $stmt->bindParam(":hwid", $hwid_hash);
          $stmt->bindParam(":subtime", $oneDayTime);
          $stmt->bindParam(":subkey", $key);

          $stmt->execute(); 

          print "success";

       }else{
        print "error";
       }

      }else{
        // check if its been used by the correct hwid.
        if(strcmp($string2,$hwid_hash) == 0 && strcmp($string1,$pcname) == 0){
         // then check if its still active or not.
         // if yes send success if not return error.

          if($string5 > time()){
            // key is active valid key
            print "success";
          }else{
            print "Key is expired.";
          }
        
        }else{
          print "wrong hwid."; // trying to use another key that doesn't have the same hwid or name
        }
      }
    }else{
      $message = "key is invalid";
      $send = send_encrypted_response($message);
      print $send;
    }

}




if (isset($_POST['pcname']) && isset($_POST['hwid']) && isset($_POST['subkey']))
{
   
   $pcname = $_POST["pcname"];
   $hwid = $_POST["hwid"];
   $subkey = $_POST["subkey"];
  
   $test = Decrypt_this_data($pcname,$encryption_key);
  // print $test . PHP_EOL;

   $test1 = Decrypt_this_data($hwid,$encryption_key);
 //  print $test1 . PHP_EOL;

   $test2 = Decrypt_this_data($subkey,$encryption_key);
 //  print $test2 . PHP_EOL;


   $servername = "localhost";
   $username = "frankoo";
   $password = "frankootest123";
   
   
   
          // try to connect to db and catches an Exception if it failed.    
          try {
           $conn = new PDO("mysql:host=$servername;dbname=edidb", $username, $password);
           // set the PDO error mode to exception
           $conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
          // echo "Connected successfully \n";
           }
       catch(PDOException $e)
           {
           echo "Connection failed: " . $e->getMessage();
           }
   
           $decrypted_pcname = Decrypt_this_data($pcname,$encryption_key);
           $decrypted_hwid = Decrypt_this_data($hwid,$encryption_key);
           $decrypted_subkey = Decrypt_this_data($subkey,$encryption_key);
           


           Add_N_checkSub($conn,$test,$test1, $test2);
   
   
}
?>