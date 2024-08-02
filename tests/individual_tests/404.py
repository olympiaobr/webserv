from base_test_script import *

def test_404_not_found():
    response = requests.get(f"{BASE_URL}/nonexistent")
    return check_status_code(response, 404)

if __name__ == "__main__":
    run_test(test_404_not_found)
