#!/opt/homebrew/bin/php
<?php
// Directory where uploaded files will be saved
$uploadDir = '../uploads/';

// Ensure the upload directory exists
if (!is_dir($uploadDir)) {
    mkdir($uploadDir, 0777, true);
}

// Check if a file is uploaded
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_FILES['uploaded_files'])) {
    $file = $_FILES['uploaded_files'];

    // Check for upload errors
    if ($file['error'] === UPLOAD_ERR_OK) {
        $uploadFilePath = $uploadDir . basename($file['name']);

        // Move the uploaded file to the designated directory
        if (move_uploaded_file($file['tmp_name'], $uploadFilePath)) {
            echo "File uploaded successfully: " . htmlspecialchars($file['name']);
        } else {
            echo "Failed to move uploaded file.";
        }
    } else {
        echo "Error during file upload: " . $file['error'];
    }
} else {
    echo "No file uploaded.";
}
?>
