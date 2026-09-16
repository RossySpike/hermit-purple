from typing import Final

from locust import HttpUser, constant, task

HOST: Final = "http://localhost:1600"  # should be env


class HermitUser(HttpUser):
    host = HOST
    wait_time = constant(0)
    abstract = True


class CursorStart(HermitUser):
    @task
    def call_api_endpoint(self):
        with self.client.get(
            "/api/image/cursor/start", catch_response=True
        ) as response:
            if response.status_code != 200:
                response.failure(f"Unexpected status code: {response.status_code}")
            elif response.elapsed.total_seconds() > 0.3:
                response.failure("Request took too long")
