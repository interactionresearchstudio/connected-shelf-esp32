// import logo from './logo.svg';
import './App.css';
import 'bootstrap/dist/css/bootstrap.min.css';
import Container from 'react-bootstrap/Container';
//import { w3cwebsocket as W3CWebSocket } from "websocket";
import React, { useState, useEffect, useRef } from 'react';
import Settings from './components/settings'

function App() {
 // const websocket = useRef(null);
 const [ value, setValue ] = useState(0); 
 const [ dataJson, setdataJson ] = useState(0); 
  useEffect(() => {
  //  websocket.current = new W3CWebSocket(`ws://${window.location.hostname}/ws`);
  //  websocket.current.onmessage = (message) => {
   //   console.log(message);
  //  };
  //  return () => websocket.current.close();
  fetch('http://192.168.2.1/status')
  .then((res) => res.json())
  .then((data) => {
    setdataJson(data);
    console.log(data);
  })
  .catch((err) => {
     console.log(err.message);
  });
  }, [])

  /*
  const onSettingsSubmit = (s) => {
    console.log("Submit settings via ws!")
    console.log(settings)
    websocket.current.send(
      JSON.stringify({
        type: "settings",
        properties: {...settings}
      })
    )
  }
  */


  return (
    <Container fluid className='my-3'>
      <h1>ConnectedStudio</h1>
      <img src="http://192.168.2.1/stream" alt="stream"/>
      <Settings startupData = {dataJson}/>
    </Container>
  );
}

export default App;
