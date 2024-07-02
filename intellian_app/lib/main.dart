import 'package:flutter/material.dart';
import 'package:geocoding/geocoding.dart';
import 'package:http/http.dart' as http;
import 'dart:convert';
import 'dart:math';
import 'package:geolocator/geolocator.dart';
import 'package:sensors_plus/sensors_plus.dart';
import 'package:permission_handler/permission_handler.dart';
import 'dart:async';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Intellian',
      theme: ThemeData(
        primarySwatch: Colors.blueGrey,
        useMaterial3: true,
      ),
      home: const MyHomePage(title: 'Intellian Home Page'),
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key, required this.title});

  final String title;

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  String status = 'Loading...';
  double sensorValue = 0.0;
  double latitude = 0.0;
  double longitude = 0.0;
  String address = '';
  double gyroscopeX = 0.0;
  double gyroscopeY = 0.0;
  double gyroscopeZ = 0.0;
  double temperature = 0.0;
  double humidity = 0.0;

  Timer? _timer;
  final List<GyroscopeEvent> _gyroEvents = [];

  @override
  void initState() {
    super.initState();
    _requestLocationPermission();
    _startUpdating();
  }

  Future<void> _requestLocationPermission() async {
    var status = await Permission.locationWhenInUse.status;
    print("Initial location permission status: $status");
    if (status.isDenied || status.isPermanentlyDenied) {
      var requestResult = await Permission.locationWhenInUse.request();
      print("Location permission request result: $requestResult");
      if (requestResult.isGranted) {
        await _getCurrentLocation();
      } else {
        setState(() {
          this.status = 'Location permissions are denied';
        });
      }
    } else if (status.isGranted) {
      await _getCurrentLocation();
    } else if (status.isRestricted) {
      setState(() {
        this.status = 'Location permissions are restricted';
      });
    } else if (status.isLimited) {
      setState(() {
        this.status = 'Location permissions are limited';
      });
    }
  }

  Future<void> _getCurrentLocation() async {
    try {
      final position = await Geolocator.getCurrentPosition(
          desiredAccuracy: LocationAccuracy.high);
      print("Current position: $position");
      setState(() {
        latitude = position.latitude;
        longitude = position.longitude;
        status = 'Location fetched successfully';
      });
      await _getAddressFromLatLng();
    } catch (e) {
      print("Error getting location: $e");
      setState(() {
        status = 'Failed to get location: $e';
      });
    }
  }

  Future<void> _getAddressFromLatLng() async {
    try {
      List<Placemark> placemarks =
          await placemarkFromCoordinates(latitude, longitude);
      Placemark place = placemarks[0];
      setState(() {
        address =
            "${place.locality}, ${place.subAdministrativeArea}, ${place.administrativeArea}, ${place.country}";
      });
    } catch (e) {
      setState(() {
        status = 'Failed to get address: $e';
      });
    }
  }

  void _startUpdating() {
    gyroscopeEvents.listen((GyroscopeEvent event) {
      _gyroEvents.add(event);
    });

    _timer = Timer.periodic(const Duration(seconds: 2), (timer) async {
      await _getCurrentLocation();

      if (_gyroEvents.isNotEmpty) {
        final lastEvent = _gyroEvents.last;
        setState(() {
          gyroscopeX = lastEvent.x;
          gyroscopeY = lastEvent.y;
          gyroscopeZ = lastEvent.z;
        });
        _gyroEvents.clear();
      }

      setState(() {
        temperature = Random().nextDouble() * 35;
        humidity = Random().nextDouble() * 100;
      });

      setState(() {
        sensorValue = Random().nextDouble() * 100;
      });
    });
  }

  void _sendDataToServer() async {
    final int timestamp = DateTime.now().millisecondsSinceEpoch;

    final data = {
      'timestamp': timestamp,
      'signal_strength': sensorValue,
      'status': 'OK',
      'latitude': latitude,
      'longitude': longitude,
      'address': address,
      'gyroscope': {'x': gyroscopeX, 'y': gyroscopeY, 'z': gyroscopeZ},
      'temperature': temperature,
      'humidity': humidity
    };

    try {
      final response = await http.post(
        Uri.parse('http://172.30.1.86:5000/api/satellite_data'),
        headers: {'Content-Type': 'application/json'},
        body: json.encode(data),
      );

      if (response.statusCode == 201) {
        setState(() {
          status = 'Data sent successfully';
        });
      } else {
        setState(() {
          status = 'Failed to send data';
        });
      }
    } catch (e) {
      setState(() {
        status = 'Error sending data: $e';
      });
    }
  }

  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.title),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
      ),
      body: Center(
        child: SingleChildScrollView(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            crossAxisAlignment: CrossAxisAlignment.center,
            children: <Widget>[
              Card(
                margin: const EdgeInsets.all(10),
                child: Padding(
                  padding: const EdgeInsets.all(10.0),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: <Widget>[
                      Text('Status: $status',
                          style: const TextStyle(fontSize: 14)),
                      const SizedBox(height: 5),
                      Text('Sensor Value: ${sensorValue.toStringAsFixed(2)}',
                          style: const TextStyle(fontSize: 12)),
                      Text('Latitude: ${latitude.toStringAsFixed(10)}',
                          style: const TextStyle(fontSize: 12)),
                      Text('Longitude: ${longitude.toStringAsFixed(10)}',
                          style: const TextStyle(fontSize: 12)),
                      Text('Address: $address',
                          style: const TextStyle(fontSize: 12)),
                      Text('Gyroscope - X: ${gyroscopeX.toStringAsFixed(10)}',
                          style: const TextStyle(fontSize: 12)),
                      Text('Gyroscope - Y: ${gyroscopeY.toStringAsFixed(10)}',
                          style: const TextStyle(fontSize: 12)),
                      Text('Gyroscope - Z: ${gyroscopeZ.toStringAsFixed(10)}',
                          style: const TextStyle(fontSize: 12)),
                      Text('Temperature: ${temperature.toStringAsFixed(2)} °C',
                          style: const TextStyle(fontSize: 12)),
                      Text('Humidity: ${humidity.toStringAsFixed(2)} %',
                          style: const TextStyle(fontSize: 12)),
                    ],
                  ),
                ),
              ),
              ElevatedButton(
                onPressed: _sendDataToServer,
                child: const Text('Send Data to Server'),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
