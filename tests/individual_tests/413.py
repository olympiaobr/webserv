from base_test_script import *

def test_413_payload_too_large_file_upload():
    # Test uploading a file that exceeds the size limit
    large_file = 'A' * (10 * 1024 * 1024)  # 10 MB file
    files = {'file': ('large_file.txt', large_file)}
    response = requests.post(f"{BASE_URL}/upload", files=files)
    return check_status_code(response, 413, "Uploading file exceeding size limit")

def test_413_payload_too_large_json_body():
    # Test sending a JSON payload that exceeds the size limit
    large_json = {'data': 'A' * (5 * 1024 * 1024)}  # 5 MB JSON payload
    response = requests.post(f"{BASE_URL}/api/data", json=large_json)
    return check_status_code(response, 413, "Sending large JSON payload")

if __name__ == "__main__":
    run_test(test_413_payload_too_large_file_upload)
    run_test(test_413_payload_too_large_json_body)
