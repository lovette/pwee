<?php
if(!extension_loaded('pwee')) {
	echo "loading pwee.so<br>";
	dl('pwee.so');
}
else
{
	echo "pwee.so already loaded<br>";
}

pwee_info();

var_dump($ff_v1); echo "<BR>";
var_dump($ff_v2); echo "<BR>";
var_dump($ff_v3); echo "<BR>";
var_dump($ff_v4); echo "<BR>";
var_dump($ff_v5); echo "<BR>";
var_dump($ff_v6); echo "<BR>";

var_dump($ff_v1); echo "<BR>";
$ff_v1++;
var_dump($ff_v1); echo "<BR>";

var_dump($prefix1_v6); echo "<BR>";
$prefix1_v6 = "hi there";
var_dump($prefix1_v6); echo "<BR>";

$httpd_v2++;
$prefix1_v2++;

var_dump(constant("SERVER_IFADDR_LO"));
echo "<BR>";
var_dump(constant("SERVER_HOSTNAME"));
echo "<BR>";

$constants = get_defined_constants();
foreach ($constants as $key=>$value)
{
	echo "$key = ";
	var_dump($value);
	echo "<BR>";
}

foreach($GLOBALS as $key=>$value)
{
	echo "\$$key = ";
	if (is_array($value))
		printf("array[%d]", count($value));
	else
		var_dump($value);
	echo "<BR>";
}
?>
