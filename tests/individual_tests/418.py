from base_test_script import *

def test_418_im_a_teapot_htcpcp_request():
    # Test HTCPCP (Hyper Text Coffee Pot Control Protocol) request
    headers = {"Accept": "application/coffee-pot-command"}
    response = requests.post(f"{BASE_URL}/teapot", headers=headers, data="START")
    return check_status_code(response, 418, "HTCPCP request to a teapot")

if __name__ == "__main__":
    run_test(test_418_im_a_teapot_htcpcp_request)
