
using UnityEngine;

public class WaterLevelController : MonoBehaviour
{
    [Header("Sensor Data")]
    public HydrationSensorData sensorData;

    [Header("Bottle Capacity")]
    public float bottleCapacityMl = 1000f;

    [Header("Water Indicator Height")]
    public float emptyHeight = 0.05f;
    public float fullHeight = 1f;

    private Vector3 initialScale;
    private Vector3 initialPosition;

    void Start()
    {
        initialScale = transform.localScale;
        initialPosition = transform.localPosition;

        if (sensorData == null)
        {
            sensorData = FindAnyObjectByType<HydrationSensorData>();
        }

        if (sensorData == null)
        {
            Debug.LogError(
                "WaterLevelController: HydrationSensorData not found!"
            );
        }
    }

    void Update()
    {
        if (sensorData == null)
            return;

        float capacity = Mathf.Max(1f, bottleCapacityMl);

        float fill = Mathf.Clamp01(
            sensorData.currentWaterMl / capacity
        );

        float height = Mathf.Lerp(emptyHeight, fullHeight, fill);

        transform.localScale = new Vector3(
            initialScale.x,
            initialScale.y * height,
            initialScale.z
        );

        transform.localPosition = new Vector3(
            initialPosition.x,
            initialPosition.y +
                (initialScale.y * height - initialScale.y) / 2f,
            initialPosition.z
        );
    }
}
