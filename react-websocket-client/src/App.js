// import logo from './logo.svg';
import './App.css';
import 'bootstrap/dist/css/bootstrap.min.css';
import { w3cwebsocket as W3CWebSocket } from "websocket";
import React, { useState, useEffect, useRef } from 'react';
import Settings from './components/settings'
import { Button, Spinner, Container, Row, Col, Tab,Tabs,Badge} from "react-bootstrap";
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
            clearInterval(scanWifiIntervalId);
            setIsWifiScanning(false);
          }   
          if ("connected" in json[0]) {
            console.log(json[0].connected);
            
            setWifiConnected(json[0].connected)
          }
          if("displayName" in json[0]){
            setName(json[0].displayName)
            console.log(json[0].displayName);
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


  function handleReset() {
        
    // Send data to the backend via POST
    fetch('http://192.168.2.1:80/control?var=reset&val=1', {  // Enter your IP address here

      method: 'GET', 
      mode: 'cors'

    })
    
  }

  const onWifiSubmit = (e) => {
    e.preventDefault();
    console.log("Wifi credentials submitted.");
    websocket.current.send(JSON.stringify({ SSID: wifiSSID, PASS: wifiPass }));
  };

  const handleWifiScan = (e) => {
    console.log("Start wifi scan");
    setIsWifiScanning(true);
    websocket.current.send("networks");
    let id = setInterval(() => {
      console.log("Requested networks");
      websocket.current.send("networks");
    }, 6000);
    setScanWifiIntervalId(id);
  };

  const handleWifiFormChange = (e) => {
    setIsWifiScanning(false);
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
            <Col>
            <h3>WiFi Settings</h3>
            </Col>
          </Row> 
          <Row>
          <Col sm="8">
          {isWifiConnected ? (<Badge bg="success" >Connected</Badge>):(<Badge bg="danger" >Not Connected</Badge>)}
          </Col>
          </Row>
          <Row>
          <Col sm="8">
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
            </Col>
            </Row>
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
        <Row>
        <Col>Once you have updated your Camera Settings, plus press Reset to return to capture mode.</Col>
        </Row>
        <Row>
          <Col>
        <Button onClick={handleReset} variant="primary">Reset</Button>
        </Col>
        </Row>
        </Container>
    </div>
  );
}

export default App;
