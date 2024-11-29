#!/opt/homebrew/bin/php
<?php

$upload_dir = __DIR__ . '../uploads/';

$request_method = $_SERVER['REQUEST_METHOD'] ?? 'CLI';


if ($request_method === 'POST') {
    if (isset($_FILES['uploaded_files']) && $_FILES['uploaded_files']['error'] === UPLOAD_ERR_OK) {
        $file_name = basename($_FILES['uploaded_files']['name']);
        $destination = $upload_dir . $file_name;

        if (move_uploaded_files($_FILES['uploaded_files']['tmp_name'], $destination)) {
            // Redirect or display confirmation
            echo "<html><body>
                <h1>File uploaded successfully!</h1>
                <p>File <b>{$file_name}</b> has been uploaded to <a href='/upload/{$file_name}' target='_blank'>View file</a>.</p>
                <a href='/upload/'>Return to uploads directory</a>
                </body></html>";
        } else {
            echo "<html><body><h1>Failed to upload file</h1></body></html>";
        }
    } else {
        echo "<html><body><h1>No file uploaded</h1></body></html>";
    }
} elseif ($request_method === 'DELETE') {
    parse_str(file_get_contents("php://input"), $data);
    $file_to_delete = isset($data['uploaded_files']) ? $data['uploaded_files'] : null;

    if ($file_to_delete) {
        $file_path = $upload_dir . $file_to_delete;

        if (file_exists($file_path)) {
            unlink($file_path);
            echo "<html><body>
                <h1>File deleted successfully!</h1>
                <a href='/upload/'>Return to uploads directory</a>
                </body></html>";
        } else {
            echo "<html><body><h1>File not found</h1></body></html>";
        }
    } else {
        echo "<html><body><h1>No file specified for deletion</h1></body></html>";
    }
} elseif (isset($_GET['list'])) {
    // List files in the upload directory
    $files = array_diff(scandir($upload_dir), array('..', '.'));
    echo "<html><body><h1>Uploaded Files</h1><ul>";
    foreach ($files as $file) {
        echo "<li><a href='/upload/{$file}'>{$file}</a></li>";
    }
    echo "</ul><a href='/'>Return to homepage</a></body></html>";
} else {
    echo "<html><body><h1>Method not supported</h1></body></html>";
}

?>


