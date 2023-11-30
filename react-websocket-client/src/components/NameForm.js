import { Form, Button } from 'react-bootstrap';

function NameForm(props) {
    return(
        <Form>
                <Form.Group className='mb-3'>
                    <Form.Label htmlFor='pass'>Device Name</Form.Label>
                    <Form.Control
                        onChange={props.onFormChange} 
                        id='name' 
                        type='text' 
                        value={props.name}
                    />
                </Form.Group>
                <Button type='submit' onClick={props.onSubmit}>Save</Button>
        </Form>
    );
}

export default NameForm;