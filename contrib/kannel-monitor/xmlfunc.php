<?php

/*
 * xmlfunc.php -- Kannel's XML status output parsing functions.
 */

function nf($number) {
    // Convert to float to handle null or string values
    return number_format((float)$number, 0, ",", ".");
}

function nfd($number) {
    // Convert to float to handle null or string values
    return number_format((float)$number, 2, ",", ".");
}

function get_timeout() {
    $refresh = isset($_REQUEST['refresh']) ? intval($_REQUEST['refresh']) : 0;
    return ($refresh > 0) ? $refresh : DEFAULT_REFRESH;
}

function get_uptime($sec) {
    // Extract seconds value if it ends with 's'
    if (is_string($sec) && preg_match('/(\d+)s$/', $sec, $matches)) {
        $sec = $matches[1];
    }
    
    // Ensure $sec is a numeric value
    if (!is_numeric($sec)) {
        return "0d 00:00:00"; // Return default if not numeric
    }
    
    $sec = (float)$sec; // Convert to float for calculations
    
    $d = floor($sec/(24*3600));
    $sec -= ($d*24*3600);
    $h = floor($sec/3600);
    $sec -= ($h*3600);
    $m = floor($sec/60);
    $sec -= ($m*60);
    
    return sprintf("%dd %dh %dm %ds", $d, $h, $m, $sec);
} 

function count_smsc_status($smscs) {
    $stats = array(
        'online' => 0,
        'disconnected' => 0,
        'connecting' => 0,
        're-connecting' => 0,
        'dead' => 0,
        'unknown' => 0
    );
    if (!is_array($smscs)) {
        return $stats;
    }
    
    foreach ($smscs as $smsc) {
        if (!isset($smsc['status'])) {
            continue;
        }
        
        // Extract just the status part (before any seconds)
        $status_parts = explode(" ", $smsc['status']);
        $status_word = $status_parts[0];
        
        foreach ($stats as $st => $i) {
            if ($status_word == $st) {
                $stats[$st]++;
                break; // Stop once we've found a match
            }
        }
    }
    return $stats;
}

function get_smscids($status, $smscs) {
    /* loop the smsc */ 
    $n = "";
    if (!is_array($smscs)) {
        return $n;
    }
    
    foreach ($smscs as $smsc) {
        if (!isset($smsc['status']) || !isset($smsc['admin-id'])) {
            continue;
        }
        
        // Extract just the status part (before any seconds)
        $status_parts = explode(" ", $smsc['status']);
        $status_word = $status_parts[0];
        
        if ($status_word == $status) {
            $n .= $smsc['admin-id']." ";
        }
    }

    return $n;
}

function format_status($st) {
    $span = 'text';
    switch ($st) {
        case "online":
        case "on-line":
            $span = 'green';
            break;
        case "disconnected":
        case "connecting":
        case "re-connecting":
            $span = 'red';
            break;
    }
    return "<span class=\"$span\">$st</span>";
}

/*
 * Parse start date, uptime and status from the status text
 */
function parse_uptime($str) {
    $regs = array();
    
    // First check for formats like "online 379990s" (SMSC format)
    if (preg_match("/(.*) (\d+)s$/", $str, $regs)) {
        $seconds = intval($regs[2]);
        $status = $regs[1];
        
        $days = floor($seconds / (24*3600));
        $seconds %= (24*3600);
        $hours = floor($seconds / 3600);
        $seconds %= 3600;
        $minutes = floor($seconds / 60);
        $seconds %= 60;
        
        $ts = ($days*24*60*60) + ($hours*60*60) + ($minutes*60) + $seconds;
        $bb_time = time()-$ts;
        $started = date("Y-m-d H:i:s", time()-$ts);
        $uptime = sprintf("%dd %02d:%02d:%02d", $days, $hours, $minutes, $seconds);
        
        return array($status, $started, $uptime);
    }
    // Then check for standard formats
    else if (preg_match("/(.*), uptime (.*)d (.*)h (.*)m (.*)s/", $str, $regs) ||
             preg_match("/(.*) (.*)d (.*)h (.*)m (.*)s/", $str, $regs)) {
        
        // Convert all values to integers to avoid non-numeric warnings
        $days = isset($regs[2]) ? intval($regs[2]) : 0;
        $hours = isset($regs[3]) ? intval($regs[3]) : 0;
        $minutes = isset($regs[4]) ? intval($regs[4]) : 0;
        $seconds = isset($regs[5]) ? intval($regs[5]) : 0;
        
        $ts = ($days*24*60*60) + ($hours*60*60) + ($minutes*60) + $seconds;
        $bb_time = time()-$ts;
        $started = date("Y-m-d H:i:s", time()-$ts);
        $uptime = sprintf("%dd %02d:%02d:%02d", $days, $hours, $minutes, $seconds);
        $status = $regs[1];
        
        return array($status, $started, $uptime);
    } 
    else {
        return array('-', '-', '-');
    }
}

/*
 * Create a link for the SMSC status with a detail popup
 */
function make_link($smsc_status, $state, $mode='red') {
    global $status, $inst;
    if ($state == 'total') {
        $smsc_status = (is_numeric($smsc_status) ? (int)$smsc_status : 0);
        return ($smsc_status > 0) ? "$smsc_status links":"none";
    } elseif (!is_array($smsc_status) || !isset($smsc_status[$state]) || $smsc_status[$state] == 0) {
        return "none";
    } else {
        switch ($mode) {
            case 'red':
                $smscids = "";
                if (isset($status[$inst]) && isset($status[$inst]['smscs'])) {
                    $smscids = get_smscids($state, $status[$inst]['smscs']);
                }
                return "<a href=\"#\" class=\"href\" onClick=\"do_alert('".
                       "smsc-ids in $state state are\\n\\n".
                       $smscids.
                       "');\"><span class=\"red\"><b>".
                       $smsc_status[$state].
                       "</b> links</span></a>";
                break;
            case 'green':
                return "<span class=\"green\"><b>".
                       $smsc_status[$state].
                       "</b> links</span>";
                break;
            default:
                return "none";
        }
    }
}

/*
 * Split the load text into 3 <TD>
 */
function split_load($str) {
    if (empty($str)) {
        return "<td>-</td><td>-</td><td>-</td>\n";
    } else {
        // Make sure str is a string before exploding
        if (!is_string($str)) {
            // Handle the case where $str might be an array or object
            if (is_array($str)) {
                // Try to convert to string if possible
                $str = implode(",", $str);
            } else {
                // Default to empty string if we can't convert
                $str = "";
            }
        }
        
        $parts = explode(",", $str);
        // Make sure we have exactly 3 parts
        while (count($parts) < 3) {
            $parts[] = "-";
        }
        
        return "<td>" . implode("</td><td>", $parts) . "</td>\n";
    }
}

/*
 * Create the admin link to change bearerbox status
 */
function admin_link($mode) {
    global $config;
    return "<a class=\"href\" href=\"#\" onClick=\"admin_url('".$mode."', ".
         "'".$config["base_url"]."/".$mode."', '".$config["admin_passwd"]."');\">".$mode."</a>";
}

/*
 * Cleanup the whole array
 */
function cleanup_array($arr) {
    if (is_array($arr) && isset($arr['gateway']) && is_array($arr['gateway'])) {
        $arr = $arr['gateway'];
        
        // Handle wdp, sms, and dlr sections
        clean_branch($arr, 'wdp');
        clean_branch($arr, 'sms');
        clean_branch($arr, 'dlr');
        
        // Handle boxes section - based on the example XML
        if (isset($arr['boxes']) && isset($arr['boxes']['box'])) {
            // If single box, make it an array
            if (isset($arr['boxes']['box']['type'])) {
                $arr['boxes'] = array($arr['boxes']['box']);
            } else {
                $arr['boxes'] = $arr['boxes']['box'];
            }
        } else if (isset($arr['boxes'][0]) && isset($arr['boxes'][0]['box'])) {
            $arr['boxes'] = $arr['boxes'][0]['box'];
        }
        
        // Handle smscs section - based on the example XML
        if (isset($arr['smscs']) && isset($arr['smscs']['smsc'])) {
            // If single smsc, make it an array
            if (isset($arr['smscs']['smsc']['id'])) {
                $arr['smscs'] = array($arr['smscs']['smsc']);
            } else {
                $arr['smscs'] = $arr['smscs']['smsc'];
            }
        } else if (isset($arr['smscs'][0]) && isset($arr['smscs'][0]['smsc'])) {
            $arr['smscs'] = $arr['smscs'][0]['smsc'];
        }
    }
    return $arr;
}

/*
 * Cleanup the branches to fold unnecessary levels
 */
function clean_branch(&$arr, $tag='') {
    $fields = array('received', 'sent');
    
    // Handle the tag (wdp, sms, dlr)
    if ($tag && isset($arr[$tag])) {
        // No need to shift if it's already in the right format
        if (isset($arr[$tag][0])) {
            $arr[$tag] = array_shift($arr[$tag]);
        }
    }
    
    // Handle received, sent fields
    foreach ($fields as $key) {
        if ($tag) {
            if (isset($arr[$tag]) && isset($arr[$tag][$key])) {
                // Check if it's already in the right format
                if (isset($arr[$tag][$key][0])) {
                    $arr[$tag][$key] = array_shift($arr[$tag][$key]);
                }
            }
        } else {
            if (isset($arr[$key])) {
                // Check if it's already in the right format
                if (isset($arr[$key][0])) {
                    $arr[$key] = array_shift($arr[$key]);
                }
            }
        }
    }
}

/*
 * Get a path/of/xml/nodes from an array
 */
function get_path($arr, $path) {
    $parts = explode("/", $path);
    if (!is_array($arr) || !is_array($parts)) {
        return 0; // Return 0 if input is not valid
    }
    
    foreach($parts as $part) {
        if (!isset($arr[$part])) {
            return 0; // Return 0 if path is not found
        }
        $arr = $arr[$part];
    }
    
    // If result is an array and has 'total' key, use that
    if (is_array($arr) && isset($arr['total'])) {
        return $arr['total'];
    }
    
    return $arr;
}
?>