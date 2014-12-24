These are tutorials for non-tunneled OpenMAX handling only for rsaspberryPI.
Someone who needs to edit or keep frames for render may refer these tutorials.

Concept is simple : Without tunneling, fill rendering buffers by triggering OpenMAX callbacks in client layer.

Advanced, OpenMAX Camera component may have multiple buffers ( default is 1 ) 
so you may implement an application like this way.

<img src="https://github.com/SonienTaegi/rpi-omx-tutorial/blob/master/docs/non-tunnel.jpg"></img>
