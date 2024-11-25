// Dynamically fetch and display uploaded files
async function fetchUploadedFiles() {
    try {
        const response = await fetch('/cgi/postdeletescript.php?list=1'); // Fetch file list from PHP
        if (response.ok) {
            const text = await response.text();
            const parser = new DOMParser();
            const doc = parser.parseFromString(text, 'text/html');
            const links = doc.querySelectorAll('li a'); // Extract file links
            const filesList = document.getElementById('uploaded-files');
            filesList.innerHTML = ''; // Clear the list

            links.forEach(link => {
                if (link.textContent !== '../') { // Ignore parent directory link
                    const li = document.createElement('li');
                    li.innerHTML = `<a href="${link.href}" target="_blank">${link.textContent}</a>`;
                    filesList.appendChild(li);
                }
            });
        } else {
            console.error('Failed to fetch uploaded files');
        }
    } catch (error) {
        console.error('Error while fetching uploaded files:', error);
    }
}

// Handle file upload form submission
document.getElementById('upload-form').addEventListener('submit', async function (event) {
    event.preventDefault();
    const formData = new FormData(this);
    try {
        const response = await fetch(this.action, {
            method: 'POST',
            body: formData,
        });
        if (response.ok) {
            alert('File uploaded successfully!');
            fetchUploadedFiles(); // Refresh file list
        } else {
            alert('Failed to upload file. Please try again.');
        }
    } catch (error) {
        console.error('Error during file upload:', error);
        alert('An error occurred while uploading the file.');
    }
});

// Handle file deletion form submission
document.getElementById('delete-form').addEventListener('submit', async function (event) {
    event.preventDefault();
    const filename = document.getElementById('delete-file-name').value;
    if (!filename) {
        alert('Please enter a file name to delete.');
        return;
    }
    try {
        const response = await fetch(this.action, {
            method: 'DELETE',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: new URLSearchParams({ file: filename }),
        });
        if (response.ok) {
            alert('File deleted successfully!');
            fetchUploadedFiles(); // Refresh file list
        } else {
            alert('Failed to delete file. Please try again.');
        }
    } catch (error) {
        console.error('Error during file deletion:', error);
        alert('An error occurred while deleting the file.');
    }
});

// Initialize: Fetch uploaded files on page load
document.addEventListener('DOMContentLoaded', function () {
    fetchUploadedFiles();
});

