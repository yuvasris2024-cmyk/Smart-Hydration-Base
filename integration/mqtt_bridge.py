
import json
import time
import socket
import paho.mqtt.client as mqtt

BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
TOPIC = "smart_hydration_base/yuvasri/sensor_data"

UNITY_HOST = "127.0.0.1"
UNITY_PORT = 5055


def on_connect(client, userdata, flags, reason_code, properties=None):
    print("Connected to MQTT broker:", reason_code)

    if reason_code == 0:
        client.subscribe(TOPIC)
        print("Subscribed to:", TOPIC)


def on_message(client, userdata, message):
    try:
        data = json.loads(message.payload.decode("utf-8"))

        print("\nHydration data received:")
        print(json.dumps(data, indent=2))

        # Forward the JSON sensor data to Unity
        payload = json.dumps(data) + "\n"

        try:
            with socket.create_connection(
                (UNITY_HOST, UNITY_PORT), timeout=3
            ) as unity_socket:
                unity_socket.sendall(payload.encode("utf-8"))

            print("Forwarded sensor data to Unity successfully.")

        except (ConnectionRefusedError, socket.timeout, OSError) as error:
            print("Unity not connected yet:", error)

    except (json.JSONDecodeError, UnicodeDecodeError) as error:
        print("Invalid MQTT message:", error)


client = mqtt.Client(
    mqtt.CallbackAPIVersion.VERSION2,
    client_id="SmartHydrationPCBridge"
)

client.on_connect = on_connect
client.on_message = on_message

while True:
    try:
        print("Connecting to MQTT...")
        client.connect(BROKER, MQTT_PORT, 60)
        client.loop_forever()

    except KeyboardInterrupt:
        print("\nBridge stopped.")
        client.disconnect()
        break

    except Exception as error:
        print("Connection error:", error)
        time.sleep(5)
