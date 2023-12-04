// import logo from './logo.svg';
import './App.css';
import 'bootstrap/dist/css/bootstrap.min.css';
import { w3cwebsocket as W3CWebSocket } from "websocket";
import React, { useState, useEffect, useRef } from 'react';
import Settings from './components/settings'
import { Button, Spinner, Container, Row, Col, Tab,Tabs } from "react-bootstrap";
import NetworksForm from "./components/NetworksForm"
import NameForm from "./components/NameForm"

function App() {
const websocket = useRef(null);
 const [ value, setValue ] = useState(0); 
 const [ dataJson, setdataJson ] = useState(0); 

 const [isWifiConnected, setWifiConnected] = useState(false);
 const [isWifiScanning, setIsWifiScanning] = useState(false);
 const [scanWifiIntervalId, setScanWifiIntervalId] = useState(null);
 const [networks, setNetworks] = useState([]);
 const [name, setName] = useState("");
 const [wifiSSID, setWifiSSID] = useState("");
 const [wifiPass, setWifiPass] = useState("");

  useEffect(() => {
    fetch('http://192.168.2.1/status')
  .then((res) => res.json())
  .then((data) => {
    setdataJson(data);
    console.log(data);
  })
  .catch((err) => {
     console.log(err.message);
  });
    websocket.current = new W3CWebSocket(`ws://${window.location.hostname}/ws`);
    websocket.current.onmessage = (e) => {
      const json = JSON.parse(e.data);
      try {
        //if (json[0].)
        // Distinguish between different ws messages from the hub
        // if networks, populate networks array
        // if status, update isAioConnected, isWifiConnected
        console.log(json);
        if (Object.prototype.toString.call(json) === "[object Array]") {
          console.log("Received array");
          if ("SSID" in json[0]) {
            console.log("Received networks!");
            setNetworks(json);
          } else {
            console.log("Array with unknown structure. No data was updated.");
          }
        }
      } catch (e) {
        // Handle error
      }
    };
    websocket.current.onopen = (e) => {
      console.log("Connected to WebSocket");
      websocket.current.send("status");
    };

    return () => websocket.current.close();
    // eslint-disable-next-line
  }, []);


  const onWifiSubmit = (e) => {
    e.preventDefault();
    console.log("Wifi credentials submitted.");
    websocket.current.send(JSON.stringify({ SSID: wifiSSID, PASS: wifiPass }));
  };

  const handleWifiScan = (e) => {
    console.log("Start wifi scan");
    setIsWifiScanning(true);
    setWifiConnected(false);
    websocket.current.send("networks");
    let id = setInterval(() => {
      console.log("Requested networks");
      websocket.current.send("networks");
    }, 6000);
    setScanWifiIntervalId(id);
  };

  const handleWifiFormChange = (e) => {
    setIsWifiScanning(false);
    setWifiConnected(false);
    clearInterval(scanWifiIntervalId);
    if (e.target.id === "ssid") {
      setWifiSSID(e.target.value);
    } else if (e.target.id === "pass") {
      setWifiPass(e.target.value);
    }
  };

  const handleNameFormChange = (e) => {
    if (e.target.id === "name") {
      setName(e.target.value);
    }
  };

  const onNameSubmit = (e) => {
    e.preventDefault();
    console.log("Name submitted.");
    websocket.current.send(JSON.stringify({ NAME: name }));
  };


  return (
    <div className="App mt-5">
      <Container>
      <Tabs
      defaultActiveKey="wifi"
      id="uncontrolled-tab-example"
      className="mb-3"
    >
      <Tab eventKey="wifi" title="Wifi Settings">
          <Col md="6" className="mt-5">
          <Row>
            <h3>WiFi Settings</h3>
          </Row>
            <Button
              variant="secondary"
              disabled={isWifiScanning}
              onClick={isWifiScanning ? null : handleWifiScan}
              className="mb-3"
            >
              {isWifiScanning ? (
                <Spinner animation="border" size="sm" as="span" />
              ) : null}
              {isWifiScanning ? " Scanning..." : "Scan for Networks"}
            </Button>
            <NetworksForm
              onSubmit={onWifiSubmit}
              networks={networks}
              onFormChange={handleWifiFormChange}
              ssid={wifiSSID}
              pass={wifiPass}
            /> 
            <NameForm
              onSubmit={onNameSubmit}
              onFormChange={handleNameFormChange}
              name={name}
            /> 

            </Col>  
            </Tab>   
            <Tab eventKey="preview" title="Preview">  
          <Col md="6" className="mt-5">
          <Row>
            <h3>Camera Preview</h3>
          </Row>
            <Row>
            <img src="http://192.168.2.1/stream" className="preview-img" alt="stream"/>  
            </Row>
          </Col>
          </Tab>
          <Tab eventKey="settings" title="Camera Settings">  
          <Col md="6" className="mt-5">
          <Row>
            <h3>Camera Settings</h3>
          </Row>
            <Settings startupData = {dataJson}/>
          </Col>
          </Tab>
        </Tabs>
        </Container>
    </div>
  );
}

export default App;
