from base_test_script import *

def test_404_not_found_nonexistent_page():
    # Test for accessing a non-existent page
    response = requests.get(f"{BASE_URL}/nonexistent-page")
    return check_status_code(response, 404, "Accessing non-existent page")

if __name__ == "__main__":
    run_test(test_404_not_found_nonexistent_page)
