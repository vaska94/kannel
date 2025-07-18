<?php
    /*
     * kannel-status.php -- display a summarized kannel status page
     *
     * This php script acts as client for the status.xml output of
     * Kannel's bearerbox and aggregates information about smsc link
     * status and message flow. It is the first step to provide an
     * external Kannel Control Center interface via HTTP.
     *
     * Stipe Tolj <stolj at kannel dot org>
     *
     * Rewrite/Makeover to update the interface and add dlr support
     * Alejandro Guerrieri <aguerrieri at kannel dot org>
     *
     * Copyright (c) 2003-2009 Kannel Group.
     *
     */

    require_once("config.php");
    require_once("xmlfunc.php");
    require_once("xmltoarray.php");

    $depth = array();
    $status = array();

    /* set php internal error reporting level */
    error_reporting(E_ALL);

    /* Refresh variables */
    $timeout = get_timeout();
    $t_down = ceil($timeout / 2);
    $t_up = ceil($timeout * 2);
    $purl = parse_url($_SERVER['REQUEST_URI']);
    $base_uri = $purl['path'];
?>

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>
<head>
    <meta http-equiv="refresh" content="<?php echo $timeout ?>; URL=<?php echo $_SERVER['REQUEST_URI'] ?>">
    <title>Kannel Status</title>
    <link href="kannel.css" rel="stylesheet" type="text/css">
    <script src="kannel.js" type="text/javascript"></script>
</head>
<body>

<!-- the header block showing some basic information -->
<table width="100%" cellspacing="0" cellpadding="0" border="0">
<tr><td valign=top>
    <h3>Kannel Status Monitor</h3>
</td><td valign=top align=left class=text>
        Current date and time: <br />
        <b><?php echo date("Y-m-d H:i:s"); ?></b>
</td><td valign=top align=right class=text>
      Refresh rate: <br />
      <a class="href" href="<?php echo $base_uri."?refresh=".$t_down; ?>"><?php echo $t_down; ?>s</a> |
      <b><?php echo $timeout; ?>s</b> |
      <a class="href" href="<?php echo $base_uri."?refresh=".$t_up; ?>"><?php echo $t_up; ?>s</a>
</td></tr>
</table>

<table width="100%" cellspacing="0" cellpadding="5" border="0">
<tr>
    <td class="text" valign="top" align="left">

<h4><?php echo sizeof($configs) ?> Configured instances</h4>

    <table width="100%" cellspacing="0" class="text" id="overall" valign="top">
    <tr>
        <th>Name</th>
        <th>Status</th>
        <th>Started</th>
        <th>Uptime</th>
        <th colspan="3" align="center">SMS (MO)</th>
        <th colspan="3" align="center">DLR (MO)</th>
        <th colspan="3" align="center">SMS (MT)</th>
        <th colspan="3" align="center">DLR (MT)</th>
        <th style="border-right: 1px solid black">Commands</th>
    </tr>
<?php
    /* loop through all configured URLs */
    foreach ($configs as $inst => $config) {
        //echo "<tr><td class=text valign=top align=left>\n";

        /* get the status.xml URL of one config */
        $url = $config["base_url"]."/status.xml?password=".$config["status_passwd"];

        $xml_data = "";

        /* open the file description to the URL */
        if (($fp = fopen($url, "r"))) {
            $bgcolor = 'green';
            /* read the XML input */
            while (!feof($fp)) {  
                $xml_data .= fread($fp, 200000);
            }
            fclose($fp);
            $xml_obj = new XmlToArray($xml_data);
            $status[$inst] = cleanup_array($xml_obj->createArray());
            if (isset($status[$inst]['smscs']) && is_array($status[$inst]['smscs'])) {
                for ($i=0; $i < sizeof($status[$inst]['smscs']); $i++) {
                    clean_branch($status[$inst]['smscs'][$i], '');
                }
            }
            /* get the status of this bearerbox */
            list ($st, $started, $uptime) = parse_uptime($status[$inst]['status']);
            /* get the info of this bearerbox into a button, to save some screen real-estate*/
            $ver = preg_replace("/\n+/", '\\n', $status[$inst]['version']);
            $ver = preg_replace("/[\'\`]/", "\'", $ver);
            $ver = 'Url: '.$config["base_url"].'\n\n'.$ver;
            $boxstatus = array (
                'name' => '<a class="href" style="color: green; font-weight: bold" href="#" onClick="alert(\''.$ver.'\'); return false;">'.$config['name'].'</a>',
                'bgcolor' => 'green',
                'status'  => $st,
                'started' => $started,
                'uptime'  => $uptime,
            );
        } else {
            $boxstatus = array (
                'name'    => $config['name'],
                'bgcolor' => 'red',
                'status'  => 'error',
                'started' => '-',
                'uptime'  => '-',
            );
        }
?>
    <tr class="<?php echo $boxstatus['bgcolor'] ?>">
        <td><?php echo $boxstatus['name'] ?></td>
        <td><?php echo $boxstatus['status'] ?></td>
        <td><?php echo $boxstatus['started'] ?></td>
        <td><?php echo $boxstatus['uptime'] ?></td>
        <?php 
        $sms_inbound = isset($status[$inst]['sms']['inbound']) ? $status[$inst]['sms']['inbound'] : '-';
        $dlr_inbound = isset($status[$inst]['dlr']['inbound']) ? $status[$inst]['dlr']['inbound'] : '-';
        $sms_outbound = isset($status[$inst]['sms']['outbound']) ? $status[$inst]['sms']['outbound'] : '-';
        $dlr_outbound = isset($status[$inst]['dlr']['outbound']) ? $status[$inst]['dlr']['outbound'] : '-';
        
        echo split_load($sms_inbound);
        echo split_load($dlr_inbound);
        echo split_load($sms_outbound);
        echo split_load($dlr_outbound);
        ?>
        <td>
            <?php echo admin_link('suspend')." | ".admin_link('isolate')." | ".admin_link('resume')." | ".admin_link('flush-dlr')."<br />"; ?>
            <?php echo admin_link('shutdown')." | ".admin_link('restart')." | ".admin_link('graceful-restart') ?>
        </td>
    </tr>
<?php
    }
?>

</table>

<h4>Overall SMS traffic</h4>

<p id="bord">
<table width="100%" cellspacing="0" class="text" id="overall" valign="top">
<tr>
    <th>Instance</th>
    <th>Received (MO)</th>
    <th>Received (DLR)</th>
    <th>Inbound (MO)</th>
    <th>Inbound (DLR)</th>
    <th>Sent (MT)</th>
    <th>Sent (DLR)</th>
    <th>Outbound (MT)</th>
    <th>Outbound (DLR)</th>
    <th>Queued (MO)</th>
    <th>Queued (MT)
    </th>
</tr>
<?php
    $sums = array(
        0, 0, 0.0, 0.0, 0, 0, 0.0, 0.0, 0, 0
    );
    foreach ($configs as $inst => $config) {
        $cols = array(
            get_path($status[$inst], 'sms/received/total'),
            get_path($status[$inst], 'dlr/received/total'),
            get_path($status[$inst], 'sms/inbound'),
            get_path($status[$inst], 'dlr/inbound'),
            get_path($status[$inst], 'sms/sent/total'),
            get_path($status[$inst], 'dlr/sent/total'),
            get_path($status[$inst], 'sms/outbound'),
            get_path($status[$inst], 'dlr/outbound'),
            get_path($status[$inst], 'sms/received/queued'),
            get_path($status[$inst], 'sms/sent/queued'),
        );
        for ($i=0;$i<10;$i++) {
            // Ensure $cols[$i] is numeric before adding
            $sums[$i] += (is_numeric($cols[$i]) ? (float)$cols[$i] : 0);
        }
?>
<tr>
    <td><?php echo $config['name'] ?></td>
    <td><?php echo nf($cols[0]) ?></td>
    <td><?php echo nf($cols[1]) ?></td>
    <td><?php echo nfd($cols[2]) ?></td>
    <td><?php echo nfd($cols[3]) ?></td>
    <td><?php echo nf($cols[4]) ?></td>
    <td><?php echo nf($cols[5]) ?></td>
    <td><?php echo nfd($cols[6]) ?></td>
    <td><?php echo nfd($cols[7]) ?></td>
    <td><?php echo nf($cols[8]) ?></td>
    <td><?php echo nf($cols[9]) ?></td>
</tr>
<?php
    }
?>
<tr class="sum">
    <td>Total</td>
    <td><?php echo nf($sums[0]) ?></td>
    <td><?php echo nf($sums[1]) ?></td>
    <td><?php echo nfd($sums[2]) ?></td>
    <td><?php echo nfd($sums[3]) ?></td>
    <td><?php echo nf($sums[4]) ?></td>
    <td><?php echo nf($sums[5]) ?></td>
    <td><?php echo nfd($sums[6]) ?></td>
    <td><?php echo nfd($sums[7]) ?></td>
    <td><?php echo nf($sums[8]) ?></td>
    <td><?php echo nf($sums[9]) ?></td>
</tr>
</table>
</p>

<h4>Box connections</h4>

<p id="bord">
<table width="100%" cellspacing="0" class="text" id="overall" valign="top">
<tr>
    <th>Instance</th>
    <th>Type</th>
    <th>ID</th>
    <th>IP</th>
    <th>Queued (MO)</th>
    <th>Started</th>
    <th>Uptime</th>
    <th style="border-right: 1px solid black">SSL</th>
</tr>
<?php
    foreach ($configs as $inst => $config) {
        /* drop an error in case we have no boxes connected */
        if (!isset($status[$inst]['boxes']) || !is_array($status[$inst]['boxes']) || empty($status[$inst]['boxes'])) {
?>
<tr>
    <td><?php echo $config['name'] ?></td>
    <td colspan="8" class="red">
        No boxes connected to this bearerbox!
    </td>
</tr>
<?php
        } else {
            $sep = ($inst > 0) ? " class=\"sep\"":'';
            /* loop the boxes */
            foreach ($status[$inst]['boxes'] as $box) {
                if (!isset($box['status'])) {
                    $st = '-';
                    $started = '-';
                    $uptime = '-';
                } else {
                    list ($st, $started, $uptime) = parse_uptime($box['status']);
                }

?>
<tr<?php echo $sep ?>>
    <td><?php echo $config['name'] ?></td>
    <td><?php echo isset($box['type']) ? $box['type'] : '-' ?></td>
    <td><?php echo isset($box['id']) ? $box['id'] : '-' ?></td>
    <td><?php echo isset($box['IP']) ? $box['IP'] : '-' ?></td>
    <td><?php echo nf(isset($box['queued']) ? $box['queued'] : (isset($box['queue']) ? $box['queue'] : 0)) ?> msgs</td>
    <td><?php echo $started ?></td>
    <td><?php echo $uptime ?></td>
    <td><?php echo isset($box['ssl']) ? $box['ssl'] : '-' ?></td>
</tr>
<?php
                $sep = '';
            }
        }
    }
?>
</table>
</p>

<h4>SMSC connections</h4>

<p id="bord">
<table width="100%" cellspacing="0" class="text" id="overall" valign="top">
<tr>
    <th>Instance</th>
    <th>Links</th>
    <th>Online</th>
    <th>Disconnected</th>
    <th>Connecting</th>
    <th>Re-Connecting</th>
    <th>Dead</th>
    <th style="border-right: 1px solid black">Unknown</th>
</tr>
<?php
    $sums = array(
        0, 0, 0, 0, 0, 0, 0
    );
    foreach ($configs as $inst => $config) {
        $smsc_status = count_smsc_status($status[$inst]['smscs']);
        $cols = array(
            array_sum($smsc_status),
            $smsc_status['online'],
            $smsc_status['disconnected'],
            $smsc_status['connecting'],
            $smsc_status['re-connecting'],
            $smsc_status['dead'],
            $smsc_status['unknown'],
        );
        for ($i=0;$i<7;$i++) {
            // Ensure $cols[$i] is numeric before adding
            $sums[$i] += (is_numeric($cols[$i]) ? (float)$cols[$i] : 0);
        }
?>
<tr>
    <td><?php echo $config['name'] ?></td>
    <td><?php echo make_link($cols[0], 'total') ?></td>
    <td><?php echo make_link($smsc_status, 'online', 'green') ?></td>
    <td><?php echo make_link($smsc_status, 'disconnected') ?></td>
    <td><?php echo make_link($smsc_status, 'connecting') ?></td>
    <td><?php echo make_link($smsc_status, 're-connecting') ?></td>
    <td><?php echo make_link($smsc_status, 'dead') ?></td>
    <td><?php echo make_link($smsc_status, 'unknown') ?></td>
</tr>
<?php
    }
?>
<tr class="sum">
    <td>Total</td>
    <td><?php echo make_link($sums[0], 'total') ?></td>
    <td><?php echo make_link($sums[1], 'total') ?></td>
    <td><?php echo make_link($sums[2], 'total') ?></td>
    <td><?php echo make_link($sums[3], 'total') ?></td>
    <td><?php echo make_link($sums[4], 'total') ?></td>
    <td><?php echo make_link($sums[5], 'total') ?></td>
    <td><?php echo make_link($sums[6], 'total') ?></td>
</tr>
</table>
</p>
<?php
    if (!empty($_REQUEST['details'])) {
?>

<h4>SMSC connection details</h4>

<p id="bord">
<table width="100%" cellspacing="0" class="text" id="overall" valign="top">
<tr>
    <th>Instance</th>
    <th>SMSC-ID</th>
    <th>Status</th>
    <th>Uptime</th>
    <th>Received (MO)</th>
    <th>Received (DLR)</th>
    <th>Sent (MT)</th>
    <th>Sent (DLR)</th>
    <th>Failed (MT)</th>
    <th>Queued (MT)</th>
    <th style="border-right: 1px solid black">Admins</th>
</tr>
<?php
        foreach ($configs as $inst => $config) {
            $sep = ($inst > 0) ? " class=\"sep\"":'';
            foreach ($status[$inst]['smscs'] as $smsc) {
                $status_parts = isset($smsc['status']) ? explode(" ", $smsc['status']) : array('-', '-');
                $st = $status_parts[0];
                $uptime_seconds = '';
                
                // Extract seconds from status string
                if (isset($status_parts[1]) && preg_match('/(\d+)s$/', $status_parts[1], $matches)) {
                    $uptime_seconds = $matches[1];
                    $uptime = get_uptime($uptime_seconds);
                } else {
                    $uptime = '-';
                }
?>
<tr<?php echo $sep ?>>
    <td><?php echo $config['name'] ?></td>
    <td>
        <?php echo isset($smsc['id']) ? $smsc['id'] : '-' ?> [<?php echo isset($smsc['admin-id']) ? $smsc['admin-id'] : '-' ?>]<br />
        <?php echo isset($smsc['name']) ? $smsc['name'] : '-' ?>
    </td>
    <td><?php echo format_status($st) ?></td>
    <td><?php echo $uptime ?></td>
    <td><?php echo isset($smsc['sms']) && isset($smsc['sms']['received']) ? nf($smsc['sms']['received']) : 
        (isset($smsc['sms'][0]['received']) ? nf($smsc['sms'][0]['received']) : '0') ?></td>
    <td><?php echo isset($smsc['dlr']) && isset($smsc['dlr']['received']) ? nf($smsc['dlr']['received']) : 
        (isset($smsc['dlr'][0]['received']) ? nf($smsc['dlr'][0]['received']) : '0') ?></td>
    <td><?php echo isset($smsc['sms']) && isset($smsc['sms']['sent']) ? nf($smsc['sms']['sent']) : 
        (isset($smsc['sms'][0]['sent']) ? nf($smsc['sms'][0]['sent']) : '0') ?></td>
    <td><?php echo isset($smsc['dlr']) && isset($smsc['dlr']['sent']) ? nf($smsc['dlr']['sent']) : 
        (isset($smsc['dlr'][0]['sent']) ? nf($smsc['dlr'][0]['sent']) : '0') ?></td>
    <td><?php echo nf(isset($smsc['failed']) ? $smsc['failed'] : 0) ?></td>
    <td><?php echo nf(isset($smsc['queued']) ? $smsc['queued'] : 0) ?></td>
    <td>
        <a class="href" href="#" onClick="admin_smsc_url('stop-smsc',
            '<?php echo $config["base_url"] ?>/stop-smsc?smsc=<?php echo $smsc['admin-id'] ?>',
            '<?php echo $smsc['admin-id'] ?>',
            '<?php echo $config["admin_passwd"] ?>'); return false;">stop</a>
        |
        <a class="href" href="#" onClick="admin_smsc_url('start-smsc',
            '<?php echo $config["base_url"] ?>/start-smsc?smsc=<?php echo $smsc['admin-id'] ?>',
            '<?php echo $smsc['admin-id'] ?>',
            '<?php echo $config["admin_passwd"] ?>'); return false;">start</a>
        <br />
        <a class="href" href="#" onClick="admin_smsc_url('remove-smsc',
            '<?php echo $config["base_url"] ?>/remove-smsc?smsc=<?php echo $smsc['admin-id'] ?>',
            '<?php echo $smsc['admin-id'] ?>',
            '<?php echo $config["admin_passwd"] ?>'); return false;">remove</a>
        |
        <a class="href" href="#" onClick="admin_smsc_url('add-smsc',
            '<?php echo $config["base_url"] ?>/add-smsc?smsc=<?php echo $smsc['admin-id'] ?>',
            '<?php echo $smsc['admin-id'] ?>',
            '<?php echo $config["admin_passwd"] ?>'); return false;">add</a>
    </td>
</tr>
<?php
            $sep = '';
            }
        }
?>
</table>
<?php
    } else {
        echo "<a class=\"href\" href=\"".$_SERVER['REQUEST_URI'];
        if (strpos($_SERVER['REQUEST_URI'], "?") > 0) {
            echo "&details=1";
        } else {
            echo "?details=1";
        }
        echo "\">SMSC connection details</a>\n";
    }
?>
</body>
</html>
