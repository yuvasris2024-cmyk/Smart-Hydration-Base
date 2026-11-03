
using UnityEngine;

public class HydrationSensorData : MonoBehaviour
{
    [Header("Live Sensor Readings")]
    public float currentWaterMl = 0f;
    public float consumedMl = 0f;
    public float temperatureC = 25f;

    [Header("Hydration Goal")]
    public float goalMl = 2500f;
    public float remainingMl = 2500f;

    [Range(0f, 100f)]
    public float waterLevelPercent = 0f;

    [Range(0f, 100f)]
    public float percentComplete = 0f;

    public int elapsedMinutes = 0;
    public string pacingStatus = "Waiting for sensor";

    [Header("Connection Status")]
    public bool sensorConnected = false;

    public void UpdateFromMqtt(
        float newCurrentWaterMl,
        float newConsumedMl,
        float newTemperatureC,
        float newGoalMl,
        float newRemainingMl,
        float newPercentComplete,
        int newElapsedMinutes,
        string newPacingStatus)
    {
        currentWaterMl = Mathf.Max(0f, newCurrentWaterMl);
        consumedMl = Mathf.Max(0f, newConsumedMl);
        temperatureC = newTemperatureC;

        goalMl = Mathf.Max(0f, newGoalMl);
        remainingMl = Mathf.Max(0f, newRemainingMl);

        percentComplete = Mathf.Clamp(
            newPercentComplete, 0f, 100f);

        // Use goal completion as the percentage for the display.
        waterLevelPercent = percentComplete;

        elapsedMinutes = Mathf.Max(0, newElapsedMinutes);
        pacingStatus = newPacingStatus ?? "Unknown";

        sensorConnected = true;
    }
}
