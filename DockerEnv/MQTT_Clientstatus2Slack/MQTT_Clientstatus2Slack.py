#!/usr/bin/env python3
# -*- coding: latin-1 -*-

import argparse
import atexit
from signal import signal, SIGABRT, SIGINT, SIGTERM

from urllib.parse import urlparse

import paho.mqtt.client as paho
from slacker import Slacker

###
#
# Provide the following values
#
# clientstatusKey='<slackerKey'
# DefaultMQTTPassword = "<mqtt password"
# DefaultMQTTUser = "<mqtt user"

from Security.Slacker import clientstatusKey
from Security.MQTT import DefaultMQTTPassword, DefaultMQTTUser


client_id = ""
debug = False
slack = Slacker(clientstatusKey)


# Define event callbacks
def on_connect(mosq, obj, flags, rc):
    print("Connect client: " + mosq._client_id.decode() + ", rc: " + str(rc),
          flush=True)
    mqttc.subscribe(("clientstatus/#", 2))
    mqttc.publish(
        "clientstatus/" + mosq._client_id.decode(), "ONLINE", 0, True
        )


def on_disconnect(mosq, obj, rc):
    print(
        "Disconnect client: " + mosq._client_id.decode() + ", rc: " + str(rc),
        flush=True
    )
    rsp = slack.chat.post_message(
        "#systemstatus",
        mosq._client_id.decode() + ": gestoppt!",
        as_user="gremmbot",
    )
    if debug:
        print(rsp)


def on_message(mosq, obj, msg):
    print(
        "Message received: "
        + msg.topic
        + ", QoS = "
        + str(msg.qos)
        + ", Payload = "
        + msg.payload.decode(),
        flush=True
    )
    rsp = slack.chat.post_message(
        "#systemstatus",
        msg.topic + ": " + msg.payload.decode(),
        as_user="gremmbot",
    )
    if debug:
        print(rsp)


def on_publish(mosq, obj, mid):
    print("mid: " + str(mid))


def on_subscribe(mosq, obj, mid, granted_qos):
    print("Subscribed: " + str(mid) + " " + str(granted_qos))


def on_unsubscribe(mosq, obj, mid):
    print("UNSubscribed: " + str(mid))


def on_log(mosq, obj, level, string):
    print(string)


@atexit.register
def cleanup_final():
    print("Cleaning up: " + client_id)
    try:
        pass
    except Exception as err:
        if debug:
            print(err)


def cleanup(sig, frame):
    print("Signal: ", str(sig), " - Cleaning up", client_id)
    mqttc.publish("clientstatus/" + client_id, "OFFLINE", 0, True)
    mqttc.disconnect()


if __name__ == "__main__":

    """ Define the Parser for the command line"""
    parser = argparse.ArgumentParser(description="MQTT Hue-Controller.")
    parser.add_argument(
        "-b",
        "--broker",
        help="IP Adress des MQTT Brokers",
        dest="mqttBroker",
        default="localhost",
    )
    parser.add_argument(
        "-p",
        "--port",
        help="Port des MQTT Brokers",
        dest="port",
        default="1883",
    )
    parser.add_argument(
        "-c",
        "--client-id",
        help="Id des Clients",
        dest="clientID",
        default="MQTT_SystemMonitor",
    )
    parser.add_argument(
        "-u",
        "--user",
        help="Broker-Benutzer",
        dest="user",
        default=DefaultMQTTUser,
    )
    parser.add_argument(
        "-P",
        "--password",
        help="Broker-Password",
        dest="password",
        default=DefaultMQTTPassword,
    )

    args = parser.parse_args()
    options = vars(args)

    for sig in (SIGABRT, SIGINT, SIGTERM):
        signal(sig, cleanup)

    client_id = options["clientID"]

    mqttc = paho.Client(client_id=client_id, clean_session=True)

    # Assign event callbacks
    mqttc.on_message = on_message
    mqttc.on_connect = on_connect
    mqttc.on_disconnect = on_disconnect
    #   mqttc.on_publish = on_publish
    #   mqttc.on_subscribe = on_subscribe
    #   mqttc.on_unsubscribe = on_unsubscribe
    #   mqttc.on_log = on_log

    # Parse CLOUDMQTT_URL (or fallback to localhost)
    # mqtt://user:password@server:port
    url_str = (
        "mqtt://"
        + options["user"]
        + ":"
        + options["password"]
        + "@"
        + options["mqttBroker"]
        + ":"
        + options["port"]
    )
    url = urlparse(url_str)

    # Connect
    mqttc.username_pw_set(url.username, url.password)
    mqttc.will_set("clientstatus/" + client_id, "OFFLINE", 0, True)
    mqttc.connect_async(url.hostname, url.port)

    mqttc.loop_forever(retry_first_connection=True)
