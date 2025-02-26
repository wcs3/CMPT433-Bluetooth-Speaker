## Configuring the BYAI as a Basic Bluetooth Speaker
The BeagleY-AI contains a [TI CC3301](https://www.ti.com/product/CC3301) chipset with Wifi 6 and Bluetooth Low Energy 5.4 capabilities. Unfortunately, the Bluetooth Low Energy specification is relatively new and driver support in Linux is limited. The BLE technology that would be relevant for our project is [LE Audio](https://www.bluetooth.com/learn-about-bluetooth/feature-enhancements/le-audio/), which only has support from the latest package and kernel versions. So, rather than trying to make the builtin bluetooth chipset work, we will use a USB Bluetooth adapter capable of Bluetooth classic audio. Unlike LE Audio, Bluetooth classic audio (A2DP) has mature Linux support. The current distro version for the BYAI, Debian 12 Bookworm, comes packaged with BlueZ version 5.66, which is capable of A2DP.

As a proof of concept/hardware test for our project, we can use some readily available packages and a python script to turn the BYAI into a basic bluetooth speaker. [This guide](https://github.com/fdanis-oss/pw_wp_bluetooth_rpi_speaker) demonstrates how to set up a Raspberry PI as a bluetooth speaker. I found that the guide applies for the BYAI as well. Here is a shortened version of the guide, tailored to the BYAI, and omitting some steps since we only want this configuration to be temporary:

1. Plug the USB Bluetooth adapter into the BYAI
2. Install PipeWire and associated packages:
   ```
   (byai)$ sudo apt install pipewire wireplumber libspa-0.2-bluetooth
   ```
3. Enable the Bluetooth controller to automatically re-pair with devices it has already been paired to:
   ```
   (byai)$ sudo sed -i 's/#JustWorksRepairing.*/JustWorksRepairing = always/' /etc/bluetooth/main.conf
   ```
4. Reboot the board:
   ```
   (byai)$ sudo reboot
   ```
5. Create a Bluetooth agent that allows pairing and tells connected bluetooth devices that we accept A2DP connections:
   1. Install python dbus bindings:
      ```
      (byai)$ sudo apt install python3-dbus
      ```
   2. Save the `speaker-agent.py` file into your home directory:
      ```
      (byai)$ wget -P ~/ 
      ```
   3. Start the speaker agent:
      ```
      (byai)$ python ~/speaker-agent.py
      ```
6. The BYAI should now be discoverable and pairable from other Bluetooth devices. Try connecting to the BYAI from your phone or laptop. By default it should show up with the same name as your board's hostname (`BeagleBone` if you haven't changed the hostname). I found that pairing/connection would sometimes require multiple attempts. If all goes well, the phone's bluetooth menu should tell you that the BYAI device is connected for audio (at least on Android, not sure how iPhone shows the connected bluetooth device's capabilities).
7. Start playing sound on your phone, then run the following command:
   ```
   (byai)$ wpctl status
   ```
   This shows you all the devices detected by PipeWire, and how they are routed to each other. Under the section `Audio: Streams:`, you should see `bluez_input.XX_XX_XX_XX_XX_XX.x` is being routed to the HDMI sound output of the BYAI. 

Now, unless you have an HMDI monitor with speakers attached to the board, you of course won't be hearing any sound output, but this at least confirms to some degree that the PipeWire configuration is working correctly.

### Getting Sound Output for Testing Purposes
To hear sound from our BYAI Bluetooth Speaker, we of course need to hook up an audio output device. The speaker jack on the zenhat doesn't come configured out of the box, and the process to get it working is quite involved. So as a first step to testing the bluetooth audio setup, we can try some other audio output methods.

A bit of a nonsensical way to get sound out of you BYAI, at least in the context of our project, is to connect it to an actual bluetooth speaker, or a pair of bluetooth headphones/earbuds. The following explains how to manually connect your BYAI to a bluetooth speaker/headphones from the terminal:
1. On the BYAI, run the bluetoothctl program:
   ```
   (byai)$ bluetoothctl
   ```
   Your prompt will change to
   ```
   [bluetooth]#
   ```
   or if you have your phone or laptop connected to the BYAI currently, the `[bluetooth]` part would show the name of the connected device, like
   ```
   [Bob's iphone]#
   ```
2. With the bluetooth speaker or headphones in pairing mode, run the `scan` command (I suggest you unpair your phone/laptop or other devices from the speaker/headphones so they don't interfere with the connection process):
   ```
   [bluetooth]# scan on
   ```
3. Nearby bluetooth devices should start showing up. Eventually, you should see your bluetooth speaker or headphones show up as a `[NEW] Device <Address> <Name>` entry, like
   ```
   ...
   [NEW] Device AB:CD:EF:GH:IJ:KL Bob's Airpods
   ...
   ```
   Take note of the address of the device.
4. Turn scanning off:
   ```
   [bluetooth]# scan off
   ```
5. Connect to the device, using the address you noted down:
   ```
   [bluetooth]# connect <Address>
   ```
   You can use tab completion when typing the address
6. Now, you should have your phone/laptop connected to the BYAI, and the BYAI connected to the bluetooth speaker/headphones. With this setup, your BYAI will essentially be acting as a bridge between the phone/laptop and speaker/headphones. Exit the `bluetoothctl` program with
   ```
   [bluetooth]# exit
   ```
   which should return you to the regular shell prompt.
7. Run
   ```
   (byai)$ wpctl status
   ```
   which should now show new entries for your speaker/headphones under the `Audio: Devices:` and `Audio: Sinks` sections. Note that each entry in the `wpctl status` tree is prefixed by an integer. This is the device's ID in PipeWire, which will be used in the next step.
8. Before you start playing audio, you should either turn the volume down on your phone/laptop, or use `wpctl` to set the volume lower on the BYAI (It defaults to max volume):
   ```
   (byai)$ wpctl set-volume <ID> <Vol>
   ```
   where `<ID>` is the ID you saw for your speaker/headphones when you ran `wpctl status`, and `<Vol>` is a number between 0 and 1, with 0 being muted and 1 being max volume.
9. Start playing audio on your phone/laptop, and you should hear it coming from the speaker/headphones!

## Enabling the Speaker Jack on the BYAI Zenhat
The speaker jack on the zenhat is attached to a [TI TLV320AIC3104](https://www.ti.com/product/TLV320AIC3104/part-details/TLV320AIC3104IRHBR) audio codec. To get it working, we need to build and install a custom device-tree overlay, and build and install a kernel driver module.

### Building and Installing a Device Tree Overlay for the TLV320AIC3104
