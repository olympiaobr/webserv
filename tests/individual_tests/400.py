from base_test_script import *

def test_400_bad_request():
    response = requests.get(f"{BASE_URL}", headers={"Host": "invalid host"})
    return check_status_code(response, 400)

if __name__ == "__main__":
    run_test(test_400_bad_request)
