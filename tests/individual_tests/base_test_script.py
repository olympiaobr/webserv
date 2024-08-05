import requests
import sys

BASE_URL = sys.argv[1] if len(sys.argv) > 1 else "http://localhost:8080"

def run_test(test_func):
    try:
        result, message = test_func()
        if result:
            print(f"PASS ✅ {message}")
        else:
            print(f"FAIL ❌ {message}")
    except Exception as e:
        print(f"ERROR: {str(e)}")

def check_status_code(response, expected_code, test_name):
    return response.status_code == expected_code, f"Expected {expected_code}, got {response.status_code} for {test_name}"