"use strict";
process.title = 'game-node';

var webSocketsServerPort = 8181;
process.env.PORT = 8181;

// websocket and http servers
var webSocketServer = require('websocket').server;
var http = require('http');

var history = [ ];
var clients = [ ];
var win = 0;

var gameboard = [];

for (var i = 0; i < 19; i++) {
	gameboard[i] = [];
	for (var j = 0; j < 19; j++) {
		gameboard[i][j] = 0;
	}
}

var server = http.createServer(function(request, response) {});

server.listen(webSocketsServerPort, function() {
    console.log(" Server listening for websocket on port " + webSocketsServerPort);
});

var wsServer = new webSocketServer({
    httpServer: server
});

wsServer.on('request', function(request) {
    var connection = request.accept(null, request.origin); 
    clients.push(connection);        
	if(clients.length > 2) {
		connection.sendUTF(JSON.stringify({type: 'more-clients'}));
    }
    history[clients.indexOf(connection)] = [];
    if (history.length > 0) {
        connection.sendUTF(JSON.stringify( { type: 'history', data: history}));
    }

    connection.on('message', function(message) {
        if(message.type == 'utf8') {
        	try {
    			var jsonObj = message.utf8Data;
	            var json = JSON.parse(jsonObj);
			} catch (e) {
	            console.log('This doesn\'t look like a valid JSON: ', jsonObj);
	            return;
	        }
	        if (json.type == 'ClientConnect') {
				console.log("Client ", clients.length);
	        	if(clients.length == 2) {
	        		console.log("Two Client Connected");
	        		clients[1].sendUTF(JSON.stringify({ type: "second-client"}));
	        		if( history.length > 1) {clients[1].sendUTF(JSON.stringify({type: "chance"}));}
	        	}
	        } 
	        else {
	        	if (json.type == 'gameboard-index') {
					console.log("Gameboard");
		            var i = json.data1;
					var j = json.data2;
					gameboard[i][j] = json.color;
					win_check(i,j, json.color);
					if(win >= 5) {
						console.log("Winner", win);
						var json_message = { type: "winner", data1: json.data1, data2: json.data2, color: json.color};
					}
					else {
						var json_message = { type: "gameboard-index", data1: json.data1, data2: json.data2, color: json.color };
						history[clients.indexOf(connection)].push(json_message);	
					}
					for (var i = 0; i < clients.length; i++) {
						clients[i].sendUTF(JSON.stringify(json_message));
						console.log("send to client ", i);
					}
		        } else {
		        	console.log('Hmm..., I\'ve never seen JSON like this: ', json);
		        }
			}
		}
    });

    connection.on('close', function() {
    	// history = [];
    	var a = clients.indexOf(connection);
    	if(a != -1) {
			history.splice(a, 1);
		}
		clients.splice(a, 1);
		console.log("Connection close client ", a);
		for (var i = 0; i < clients.length; i++) {
			clients[i].sendUTF(JSON.stringify({type: "close-win", color: a}));
		}
    });
});


function win_check(row, col, usr_value) {
    win = 0;

	//forward vertically
	forward_check(row, col, 1, usr_value);

	// backward vertically
	backward_check(row, col, 1, usr_value);

    if(win >= 5) {
    	return;
    }
    else {
    	win = 0;
    }

	//forward horizontally
	forward_check(col, row, 0, usr_value);

	// backward horizontally
	backward_check(col, row, 0, usr_value);

    if(win >= 5) {
    	return;
    }
    else {
    	win = 0;
    }

	//diagonal forward downwards check
	diagonal_forward_down(row, col, usr_value);

	//diagonal backward upwards check
	diagonal_backward_up(row, col, usr_value);

    if(win >= 5) {
    	return;
    }
    else {
    	win = 0;
    }
    
    //diagonal forward upwards check
	diagonal_forward_up(row, col, usr_value);

    //diagonal backward downward check
	diagonal_backward_down(row, col, usr_value);
	
    if(win >= 5) {
    	return;
    }
    else {
    	win = 0;
    }
}

// first argument tells the searching - horizon or vertical and second argument for gameboard.
function forward_check(dir, value, pro, usr) {
	//forward horizontally
    if(dir <= 14) {
    	var next = 4;
    }
    else {
    	var next = 18 - dir;
    }

    if(pro == 1) {
	    for(var c = dir; c <= dir+next; c++) {
	    	// console.log("forward vertical", dir , value, usr);
	    	if(gameboard[c][value] == 0 || gameboard[c][value] != usr) {
	    		// console.log("break");
	    		break;
	    	}
	    	if(gameboard[c][value] == usr ) {
	    		win++;
	    	}
	    }	
    }
    else {
 	   for(var c = dir; c <= dir+next; c++) {
	    	// console.log("forward horizontal", value , dir, usr);
	    	if(gameboard[value][c] == 0 || gameboard[value][c] != usr) {
	    		// console.log("break");
	    		break;
	    	}
	    	if(gameboard[value][c] == usr ) {
	    		win++;
	    	}
	    }	
    }
    if(win == 1) {
    	win = 0;
    }
}

// first argument tells the searching - horizon or vertical and second argument for gameboard.
function backward_check(dir, value, pos, usr) {
    //backward horizontally
    if (dir >= 4) {
    	var previous = 4;
    }
    else {
    	previous = dir;
    }

    if(pos == 1) {
		for(var c = dir; c >= dir-previous; c--) {
	    	// console.log("backward vertical", dir , value, usr);
	    	if(gameboard[c][value] == 0 || gameboard[c][value] != usr) {
	    		// console.log("break");
	    		break;
    		}
	    	if(gameboard[c][value] == usr ) {
	    		win++;
	    	}
    	}
    }
    else {
		for(var c = dir; c >= dir-previous; c--) {
	    	// console.log("backward horizontal", value , dir, usr);
	    	if(gameboard[value][c] == 0 || gameboard[value][c] != usr) {
	    		// console.log("break");
	    		break;
    		}
	    	if(gameboard[value][c] == usr ) {
	    		win++;
	    	}
    	}   
    }
}

function diagonal_forward_down(row, col, usr) {
	var fun_win = 0;
	for(var c = 0; c <= 4 ; c++) {
		if(row + c <= 18 && col + c <= 18) {
			// console.log("diagonal forward down", row, col, usr);
			if(gameboard[row+c][col+c] == 0 || gameboard[row+c][col+c] != usr) {
	    		// console.log("break");
	    		break;
    		}
	    	if(gameboard[row+c][col+c] == usr ) {
	    		fun_win++;
	    		win++;
	    	}
		}
	}
	if(fun_win == 5) {
		win = 5;
		return;
	}
	if(win >= 1) {
		win--;
	}
}

function diagonal_backward_up(row, col, usr) {
	var fun_win = 0;
	for(var c = 0; c <= 4 ; c++) {
		if(row-c >= 0 && col - c >= 0) {
			// console.log("diagonal backward up", row, col, usr);
			if(gameboard[row-c][col-c] == 0 || gameboard[row-c][col-c] != usr) {
	    		// console.log("break");
	    		break;
    		}
	    	if(gameboard[row-c][col-c] == usr ) {
	    		fun_win++;
	    		win++;
	    	}
		}
	}
	if(fun_win == 5) {
		win = 5;
		return;
	}
}

function diagonal_backward_down(row, col, usr) {
	var fun_win = 0;
	for(var c = 0; c <= 4 ; c++) {
		if(row + c <= 18 && col - c >= 0) {
			// console.log("diagonal backward down", row, col, usr);
			if(gameboard[row+c][col-c] == 0 || gameboard[row+c][col-c] != usr) {
	    		// console.log("break");
	    		break;
    		}
	    	if(gameboard[row+c][col-c] == usr ) {
	    		win++;
	    		fun_win++;
	    	}
		}
	}
	if(fun_win == 5) {
		win = 5;
		return;
	}
}

function diagonal_forward_up(row, col, usr) {
	var fun_win = 0;
	for(var c = 0; c <= 4 ; c++) {
		if(row - c >= 0 && col + c <= 18) {
			// console.log("diagonal forward up", row, col, usr);
			if(gameboard[row-c][col+c] == 0 || gameboard[row-c][col+c] != usr) {
	    		// console.log("break");
	    		break;
    		}
	    	if(gameboard[row-c][col+c] == usr ) {
	    		win++;
	    		fun_win++;
	    	}
		}
	}
	if(fun_win == 5) {
		win = 5;
		return;
	}
	if(win >= 1) {
		win--;
	}
}