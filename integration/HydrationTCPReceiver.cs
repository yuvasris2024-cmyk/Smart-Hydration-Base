
using System;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using UnityEngine;

public class HydrationTCPReceiver : MonoBehaviour
{
    [Header("TCP Settings")]
    public int port = 5055;

    [Header("Sensor Data Component")]
    public HydrationSensorData sensorData;

    private TcpListener listener;
    private Thread listenerThread;
    private volatile bool running;

    private readonly object dataLock = new object();
    private string latestJson;
    private string errorMessage;

    [Serializable]
    private class HydrationMessage
    {
        public float currentWaterMl;
        public float consumedMl;
        public float temperatureC;
        public float goalMl;
        public float remainingMl;
        public float percentComplete;
        public int elapsedMinutes;
        public string pacingStatus;
    }

    void Start()
    {
        if (sensorData == null)
        {
            sensorData = GetComponent<HydrationSensorData>();
        }

        if (sensorData == null)
        {
            Debug.LogError(
                "HydrationSensorData is missing! " +
                "Attach it to ElectronicsManager and assign it here."
            );
        }

        running = true;

        listenerThread = new Thread(ListenForPython);
        listenerThread.IsBackground = true;
        listenerThread.Start();

        Debug.Log(
            "Hydration TCP receiver starting on port " + port
        );
    }

    void ListenForPython()
    {
        try
        {
            listener = new TcpListener(IPAddress.Loopback, port);
            listener.Start();

            Debug.Log(
                "Hydration TCP receiver LISTENING on port " + port
            );

            while (running)
            {
                using (TcpClient client = listener.AcceptTcpClient())
                using (NetworkStream stream = client.GetStream())
                using (StreamReader reader =
                       new StreamReader(stream, Encoding.UTF8))
                {
                    Debug.Log("Python bridge connected to Unity!");

                    string line;

                    while (running && (line = reader.ReadLine()) != null)
                    {
                        lock (dataLock)
                        {
                            latestJson = line;
                        }
                    }
                }
            }
        }
        catch (Exception ex)
        {
            if (running)
            {
                lock (dataLock)
                {
                    errorMessage = ex.ToString();
                }
            }
        }
    }

    void Update()
    {
        string json;
        string error;

        lock (dataLock)
        {
            json = latestJson;
            latestJson = null;

            error = errorMessage;
            errorMessage = null;
        }

        if (!string.IsNullOrEmpty(error))
        {
            Debug.LogError(
                "Hydration TCP receiver error:\n" + error
            );
        }

        if (!string.IsNullOrEmpty(json))
        {
            try
            {
                HydrationMessage message =
                    JsonUtility.FromJson<HydrationMessage>(json);

                if (message == null)
                {
                    Debug.LogError("Could not read hydration JSON.");
                    return;
                }

                if (sensorData != null)
                {
                    sensorData.UpdateFromMqtt(
                        message.currentWaterMl,
                        message.consumedMl,
                        message.temperatureC,
                        message.goalMl,
                        message.remainingMl,
                        message.percentComplete,
                        message.elapsedMinutes,
                        message.pacingStatus
                    );
                }

                Debug.Log(
                    "Unity received hydration data: " +
                    "Current Water = " + message.currentWaterMl + " mL, " +
                    "Consumed = " + message.consumedMl + " mL, " +
                    "Temperature = " + message.temperatureC + " C, " +
                    "Goal = " + message.goalMl + " mL, " +
                    "Remaining = " + message.remainingMl + " mL, " +
                    "Progress = " + message.percentComplete + "%, " +
                    "Status = " + message.pacingStatus
                );
            }
            catch (Exception ex)
            {
                Debug.LogError(
                    "Invalid hydration JSON: " + ex.Message
                );
            }
        }
    }

    void OnDestroy()
    {
        running = false;

        try
        {
            listener?.Stop();
        }
        catch (Exception)
        {
            // The listener may already be stopped.
        }

        if (listenerThread != null && listenerThread.IsAlive)
        {
            listenerThread.Join(500);
        }
    }
}
