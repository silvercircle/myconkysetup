#!/usr/bin/php

<?php

function degToCompass($num)
{
    if($num < 0 || $num > 360)
        $num = 0;

    $val = (int)(($num/22.5)+.5);

    $arr = array("N","NNE","NE","ENE","E","ESE", "SE", "SSE","S","SSW","SW","WSW","W","WNW","NW","NNW");

    return $arr[($val % 16)];
}

function test_image_daytime($icon) {
	switch ($icon) {
		case "clear-day":
			return 'a';
		case "clear-night":
			return 'a';
		case "rain":
			return 'g';
		case "snow":
			return 'o';
		case "sleet":
			return 'x';
		case "wind":
			return '9';
		case "fog":
			return '7';
		case "cloudy":
			return 'e';
		case "partly-cloudy-day":
			return 'c';
		case "partly-cloudy-night":
			return 'a';
    }
}

function test_image_nighttime($icon) {
    switch($icon) {
		case "clear-day":
			return 'A';
		case "clear-night":
			return 'A';
		case "rain":
			return 'G';
		case "snow":
			return 'O';
		case "sleet":
			return 'x';
		case "wind":
			return '9';
		case "fog":
			return '7';
		case "cloudy":
			return 'f';
		case "partly-cloudy-day":
			return 'C';
		case "partly-cloudy-night":
			return 'C';
    }
}

function test_image_forecast($icon) {
	switch ($icon) {
		case "clear-day":
			return 'a';
		case "clear-night":
			return 'a';
		case "rain":
			return 'j';
		case "snow":
			return 'q';
		case "sleet":
			return 'x';
		case "wind":
			return '9';
		case "fog":
			return '7';
		case "cloudy":
			return 'e';
		case "partly-cloudy-day":
			return 'c';
		case "partly-cloudy-night":
			return 'a';
	}
}

# Put your coordinates here:
$coords="48.2082,16.3738";

# Put your API key for Dark Sky here:
$apiKey="8b828fcdd7c329b4afee24fff3493dbb";

# Put your preferred temperature units here:
$units="si";


# Get current weather forecast:
$jsonData=`curl https://api.darksky.net/forecast/{$apiKey}/{$coords}?units=${units}`;

# Decode json object:
$weatherData=json_decode($jsonData);

# Generate current icon based on time of day:
$hour=date("H");

if ($hour > 6 && $hour < 18) {
	echo(test_image_daytime($weatherData->currently->icon));
	echo("\n");
}
else {
	echo(test_image_nighttime($weatherData->currently->icon));
	echo("\n");
}

# Write apparent current temperature:
echo((int)round($weatherData->currently->apparentTemperature));
echo("\n");


# Write forecast information for the next three days:
$timeZone = $weatherData->timezone;
$dt = new DateTime("now", new DateTimeZone($timeZone));

for ($i = 0; $i < 3; ++$i) {
	echo(test_image_forecast($weatherData->daily->data[( $i+1 )]->icon));
	echo("\n");
	echo((int)round($weatherData->daily->data[( $i+1 )]->apparentTemperatureLow));
	echo("\n");
	echo((int)round($weatherData->daily->data[( $i+1 )]->apparentTemperatureHigh));
	echo("\n");
    $dt->setTimestamp($weatherData->daily->data[( $i+1 )]->time);
	echo($dt->format("D"));
	echo("\n");
}
echo((int)round($weatherData->currently->temperature));
echo("\n");
echo("Dew point: ". $weatherData->currently->dewPoint);
echo("\n");
echo("Humidity: " . (float)($weatherData->currently->humidity)*100);
echo("\n");
echo((int)round($weatherData->currently->pressure));
echo("\n");
echo((int)round($weatherData->currently->windSpeed)); // 20
echo("\n");
echo("UV: " . $weatherData->currently->uvIndex); // 21
echo("\n");
echo("Vis: " . (int)round($weatherData->currently->visibility)); // 22
echo("\n");
echo(date("H:i", $weatherData->daily->data[0]->sunriseTime)); // 23
echo("\n");
echo(date("H:i", $weatherData->daily->data[0]->sunsetTime)); // 24
echo("\n");
echo(degToCompass($weatherData->currently->windBearing)); // 25
echo("\n");
echo(date("H:i", $weatherData->currently->time)); // 26
echo("\n");
echo($weatherData->currently->summary); // 27
echo("\n");
echo($weatherData->timezone); // 28
echo("\n");
?>
