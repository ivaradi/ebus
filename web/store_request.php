<?php

$requestDir = "/home/ivaradi/www/homeauto/data/requests";

if (isset($_GET["ts"]) && preg_match("/^[1-9][0-9]+$/", $_GET["ts"]) &&
    isset($_GET["relayOn"]) && ($_GET["relayOn"]=="true" ||
                                $_GET["relayOn"]=="false") &&
    isset($_GET["ledOn"]) && ($_GET["ledOn"]=="true" ||
                              $_GET["ledOn"]=="false"))
{
    $ts = $_GET["ts"];
    $relayOn = $_GET["relayOn"];
    $ledOn = $_GET["ledOn"];

    foreach(glob($requestDir . "/*request.*") as $path) {
        if (is_file($path)) {
            unlink($path);
        }
    }

    $data = "{\"relayOn\": " . $relayOn . ", \"ledOn\": " . $ledOn . "}";

    $requestPath = $requestDir . "/request." . $ts;
    $tmpRequestPath = $requestDir . "/tmp.request." . $ts;
    $fh = fopen($tmpRequestPath, "w");
    if (!$fh) {
        header("HTTP/1.0 500 Internal Server Error");
        exit(print_r(error_get_last(), true));
    }
    fwrite($fh, $data);
    fclose($fh);

    rename($tmpRequestPath, $requestPath);

    echo "OK";
} else {
    echo "Invalid and/or missing parameters!";
}
