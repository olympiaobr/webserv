#!/opt/homebrew/bin/php
<?php
// Ensure error reporting is enabled
error_reporting(E_ALL);
ini_set('display_errors', 1);

// Directory where uploaded files will be saved
$uploadDir = getenv('UPLOAD_DIR');
if (!$uploadDir) {
    die("UPLOAD_DIR environment variable is not set.");
}

// Ensure the upload directory exists
if (!is_dir($uploadDir)) {
    mkdir($uploadDir, 0755, true);
}

// Content-Type header
header("Content-Type: text/html");

// Check if the request method is POST
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    echo "No content to read.";
    exit;
}

// Check if files are uploaded
if (!isset($_FILES['uploaded_files'])) {
    echo "No files uploaded.";
    exit;
}

$uploadedFiles = [];
$errors = [];
$files = $_FILES['uploaded_files'];

// Handle both single and multiple file uploads
if (is_array($files['name'])) {
    $fileCount = count($files['name']);
} else {
    $fileCount = 1;
}

for ($i = 0; $i < $fileCount; $i++) {
    // Extract file properties
    $fileName = is_array($files['name']) ? $files['name'][$i] : $files['name'];
    $fileTmpName = is_array($files['tmp_name']) ? $files['tmp_name'][$i] : $files['tmp_name'];
    $fileError = is_array($files['error']) ? $files['error'][$i] : $files['error'];

    // Validate file name
    $fileName = basename($fileName);
    if (empty($fileName)) {
        $errors[] = "Invalid file name.";
        continue;
    }

    // Sanitize file name
    $fileName = preg_replace("/[^a-zA-Z0-9._-]/", "_", $fileName);

    // Limit file size (100MB)
    $maxFileSize = 100 * 1024 * 1024; // 100MB
    if (filesize($fileTmpName) > $maxFileSize) {
        $errors[] = "File '{$fileName}' exceeds the maximum allowed size of 100MB.";
        continue;
    }

    // Check if the file already exists
    $uploadFilePath = $uploadDir . DIRECTORY_SEPARATOR . $fileName;
    if (file_exists($uploadFilePath)) {
        $errors[] = "File '{$fileName}' already exists.";
        continue;
    }

    // Attempt to move the uploaded file
    if ($fileError === UPLOAD_ERR_OK) {
        if (move_uploaded_file($fileTmpName, $uploadFilePath)) {
            $uploadedFiles[] = $fileName;
        } else {
            $errors[] = "Error saving file '{$fileName}'.";
        }
    } else {
        $errors[] = "Error uploading file '{$fileName}'.";
    }
}

// Display the results
if (!empty($uploadedFiles)) {
    echo "<h2>Uploaded Files:</h2>";
    echo "<ul>";
    foreach ($uploadedFiles as $fileName) {
        echo "<li>" . htmlspecialchars($fileName) . "</li>";
    }
    echo "</ul>";
}

if (!empty($errors)) {
    echo "<h2>Errors:</h2>";
    echo "<ul>";
    foreach ($errors as $error) {
        echo "<li>{$error}</li>";
    }
    echo "</ul>";
}
?>
