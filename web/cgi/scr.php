#!/opt/homebrew/bin/php
<?php
// Enable error reporting for debugging
error_reporting(E_ALL);
ini_set('display_errors', 1);

// Retrieve the "username" value from the POST request
$username = isset($_POST['username']) ? htmlspecialchars($_POST['username']) : null;

// Check if a username was provided
if ($username) {
    echo <<<HTML
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Welcome</title>
        <link rel="stylesheet" href="/css/styles.css">
    </head>
    <body>
        <h1>Welcome, $username!</h1>
        <p>Thank you for submitting your username.</p>
        <a href="/">Return to homepage</a>
    </body>
    </html>
    HTML;
} else {
    echo <<<HTML
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Error</title>
        <link rel="stylesheet" href="/css/styles.css">
    </head>
    <body>
        <h1>Error</h1>
        <p>No username provided. Please go back and enter a username.</p>
        <a href="/">Return to homepage</a>
    </body>
    </html>
    HTML;
}
?>
