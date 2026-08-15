async def test_liveness_and_readiness(service_client):
    live = await service_client.get("/health/live")
    assert live.status == 200
    assert live.json() == {"status": "ok"}

    ready = await service_client.get("/health/ready")
    assert ready.status == 200
    assert ready.json() == {
        "status": "ready",
        "checks": {"postgres": True, "redis": True},
    }


async def test_ping(service_client):
    response = await service_client.get("/ping")
    assert response.status == 200
