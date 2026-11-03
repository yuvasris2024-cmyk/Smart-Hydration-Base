
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class HydrationDashboardUI : MonoBehaviour
{
    [Header("Sensor Data")]
    public HydrationSensorData sensorData;

    [Header("Dashboard Text")]
    public TMP_Text readingsText;

    [Header("Progress Bar")]
    public Slider hydrationProgressBar;

    void Start()
    {
        if (sensorData == null)
        {
            HydrationTCPReceiver receiver =
                FindAnyObjectByType<HydrationTCPReceiver>();

            if (receiver != null)
            {
                sensorData = receiver.sensorData;
            }

            if (sensorData == null)
            {
                sensorData =
                    FindAnyObjectByType<HydrationSensorData>();
            }
        }

        if (readingsText == null)
        {
            Debug.LogError(
                "Assign SensorReadings to the Readings Text field!"
            );
        }

        if (sensorData == null)
        {
            Debug.LogError(
                "HydrationSensorData was not found in the scene!"
            );
        }

        if (hydrationProgressBar == null)
        {
            Debug.LogWarning(
                "Assign HydrationProgressBar to the Progress Bar field."
            );
        }
        else
        {
            hydrationProgressBar.minValue = 0f;
            hydrationProgressBar.maxValue = 100f;
            hydrationProgressBar.interactable = false;
        }
    }

    void Update()
    {
        if (sensorData == null)
            return;

        if (readingsText != null)
        {
            string connectionStatus = sensorData.sensorConnected
                ? "Connected"
                : "Waiting for sensor";

            readingsText.text =
                "SMART HYDRATION BASE\n\n" +
                "Current Water: " +
                sensorData.currentWaterMl.ToString("F1") + " mL\n" +
                "Consumed: " +
                sensorData.consumedMl.ToString("F1") + " mL\n" +
                "Temperature: " +
                sensorData.temperatureC.ToString("F1") + " C\n" +
                "Daily Goal: " +
                sensorData.goalMl.ToString("F0") + " mL\n" +
                "Remaining: " +
                sensorData.remainingMl.ToString("F0") + " mL\n" +
                "Progress: " +
                sensorData.percentComplete.ToString("F1") + "%\n" +
                "Elapsed: " +
                sensorData.elapsedMinutes + " minutes\n" +
                "Status: " + sensorData.pacingStatus + "\n" +
                "Connection: " + connectionStatus;
        }

        if (hydrationProgressBar != null)
        {
            hydrationProgressBar.value =
                Mathf.Clamp(sensorData.percentComplete, 0f, 100f);
        }
    }
}
