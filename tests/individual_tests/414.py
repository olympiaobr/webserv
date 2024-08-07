from base_test_script import *

def test_414_uri_too_long_get_request():
    # Test GET request with an extremely long query string
    long_query = '&'.join([f'param{i}=value{i}' for i in range(1000)])
    response = requests.get(f"{BASE_URL}/api/search?{long_query}")
    return check_status_code(response, 414, "GET request with extremely long query string")

if __name__ == "__main__":
    run_test(test_414_uri_too_long_get_request)
    run_test(test_414_uri_too_long_post_request)
