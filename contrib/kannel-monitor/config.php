<?php
    /*
     * Configure the kannel instances here
     * This is a sample configuration - replace with your actual instances
     */
    $configs = array(
        array( "base_url" => "http://localhost:13000",
               "status_passwd" => "your_status_password",
               "admin_passwd" => "your_admin_password", 
               "name" => "Main Gateway"
             ),
        array( "base_url" => "http://kannel-backup:13000",
               "status_passwd" => "your_status_password",
               "admin_passwd" => "your_admin_password",
               "name" => "Backup Gateway"
             ),
        array( "base_url" => "http://kannel-test:13000",
               "status_passwd" => "your_status_password",
               "admin_passwd" => "your_admin_password",
               "name" => "Test Gateway"
             )
    );

    /* some constants */
    define('MAX_QUEUE', 100); /* Maximum size of queues before displaying it in red */
    define('DEFAULT_REFRESH', 60); /* Default refresh time for the web interface */
?>