from typing import Final

from locust import HttpUser, constant, task

HOST: Final = "http://localhost:1600"
ENDPOINT: Final = "/api/image/cursor/start"


class APIUser(HttpUser):
    host = HOST
    wait_time = constant(0)
    # wait_time = between(1, 3)

    @task
    def call_api_endpoint(self):
        response = self.client.get(ENDPOINT)
        if response.status_code != 200:
            response.failure(f"Unexpected status code: {response.status_code}")
