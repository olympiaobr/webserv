from base_test_script import *

def test_505_http_version_not_supported_old_version():
    # Test using an old HTTP version
    session = requests.Session()
    session.mount("http://", requests.adapters.HTTPAdapter(max_retries=0))
    response = session.get(f"{BASE_URL}", headers={"Accept": "HTTP/1.0"})
    return check_status_code(response, 505, "Using unsupported HTTP/1.0")

def test_505_http_version_not_supported_future_version():
    # Test using a future (non-existent) HTTP version
    session = requests.Session()
    session.mount("http://", requests.adapters.HTTPAdapter(max_retries=0))
    response = session.get(f"{BASE_URL}", headers={"Accept": "HTTP/3.0"})
    return check_status_code(response, 505, "Using unsupported HTTP/3.0")

if __name__ == "__main__":
    run_test(test_505_http_version_not_supported_old_version)
    run_test(test_505_http_version_not_supported_future_version)
