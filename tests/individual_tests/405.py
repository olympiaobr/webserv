from base_test_script import *

def test_405_method_not_allowed_post_to_get_only():
    # Test POST request to a GET-only endpoint
    response = requests.post(f"{BASE_URL}/css")
    return check_status_code(response, 405, "POST request to GET-only endpoint")

def test_405_method_not_allowed_delete_to_read_only():
    # Test DELETE request to a read-only resource
    response = requests.delete(f"{BASE_URL}/js/script.js")
    return check_status_code(response, 405, "DELETE request to read-only resource")

if __name__ == "__main__":
    run_test(test_405_method_not_allowed_post_to_get_only)
    run_test(test_405_method_not_allowed_delete_to_read_only)
