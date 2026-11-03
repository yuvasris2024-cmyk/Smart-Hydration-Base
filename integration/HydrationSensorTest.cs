
using UnityEngine;
using UnityEngine.InputSystem;

public class HydrationSensorTest : MonoBehaviour
{
    public HydrationSensorData sensorData;

    void Start()
    {
        if (sensorData == null)
        {
            sensorData = GetComponent<HydrationSensorData>();
        }

        if (sensorData == null)
        {
            Debug.LogError(
                "HydrationSensorData is missing from ElectronicsManager!"
            );
        }
    }

    void Update()
    {
        // Press T to test sensor data using the new Input System.
        if (Keyboard.current != null &&
            Keyboard.current.tKey.wasPressedThisFrame)
        {
            if (sensorData == null)
            {
                Debug.LogError("Sensor data component is not assigned.");
                return;
            }

            sensorData.UpdateFromMqtt(
                850f,
                250f,
                25.7f,
                2500f,
                2250f,
                10f,
                30,
                "Hydration test"
            );

            Debug.Log("Hydration sensor test completed!");
        }
    }
}
