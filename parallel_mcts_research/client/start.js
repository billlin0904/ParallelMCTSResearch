var usr = 1; // means black color
var chance = true;

// inintalize whole board
var gameboard = [];

var MAX_WIDTH = 10;
var MAX_HEIGHT = 10;

for (var i=0; i<MAX_WIDTH; i++) {
	gameboard[i] = [];
	for (var j=0; j<MAX_HEIGHT; j++) {
		gameboard[i][j] = 0;
	}
}

window.onload = function() {
    var canvas = document.getElementById("gameboard");
	var context = canvas.getContext('2d');	
    var connection = new WebSocket('ws://127.0.0.1:9090');
    	
    connection.onopen = function () {
		connection.send(JSON.stringify({packet: { cmd: 1000 } }));
        initalize_area();
        document.getElementById("status").innerHTML = "Black Stones. You play first.";
        document.getElementById("stones").innerHTML = "BLACK";
    };

    connection.onerror = function (error) {
        console.log('WebSocket Error ' + error);
    };

    connection.onmessage = function (message) {
        try {
            var json = JSON.parse(message.data);
		} catch (e) {
            return;
        }

        if (json.packet.cmd == 1002) {
            var i = json.packet.move.row;
			var j = json.packet.move.column;
			var color = json.packet.player_id;
			//updateGameTree(json.mcts_result);
			gameboard[i][j] = color;
			drawnodes(i, j, color);
			if(json.color != usr) {
				chance = true;
				document.getElementById("status").innerHTML = "Your turn.";
			}
		} else if (json.packet.cmd == 1001) {
			
        } else {
        	if (json.type == 'second-client') {
        		document.getElementById("status").innerHTML = "White Stones. You play second.";
        		document.getElementById("stones").innerHTML = "WHITE";
        		usr = -usr;
        		chance = false;
        	}
        	else {
        		if(json.type == 'winner') {
        			document.getElementById("status").style.color = "#C2185B";
        			var i = json.data1;
					var j = json.data2;
					var color = json.color;
					gameboard[i][j] = json.color;
					drawnodes(i, j, color);
        			if(json.color == 1) {
        				document.getElementById("status").innerHTML = "Black stones player wins";	
        			}
        			else {
        				document.getElementById("status").innerHTML = "White stones player wins";
        			}
        			disableScreen();
        		}
        		else {
		        	if (json.type === 'history') {
		        		for (var i = 0; i < json.data.length; i++) {
		        			for (var k = 0; k < json.data[i].length; k++) {
		            			var ro = json.data[i][k].data1;
		            			var co = json.data[i][k].data2;
		            			gameboard[ro][co] = json.data[i][k].color;
								drawnodes(json.data[i][k].data1, json.data[i][k].data2, json.data[i][k].color);
							}
						}
		        	}
		    		else {
		    			if(json.type == 'more-clients') {
		    				document.getElementById("more-client").innerHTML = "Just Watch..";
		    				document.getElementById("status").style.color = "white";
		    				document.getElementById("stones").style.color = "white";
		    				disableScreen();
		    			}
		    			else {
		    				if(json.type == 'chance'){
		    					chance = true;
		    				}
		    				else{
		    					if(json.type == 'close-win') {
		    						if(json.color == 0) {
				        				document.getElementById("status").style.color = "#C2185B";
				        				document.getElementById("status").innerHTML = "White stones player wins";
				        				disableScreen();
				        			}
				        			if(json.color == 1) {
				        				document.getElementById("status").style.color = "#C2185B";
		    							document.getElementById("status").innerHTML = "Black stones player wins";
				        				disableScreen();
				        			}
		    					}
		    					else
		    						console.log('Hmm..., I\'ve never seen JSON like this: ', json);
		    				}
		    			}
		            }
		        }
        	}
        }
    };
	
	function updateGameTree(mcts_result) {
		var svg = d3.select('svg');
	    var width = +svg.attr("width");
	    var height = +svg.attr("height");
	    var g = svg.append("g").attr("transform", "translate(40, 0)");
		var tree = d3.tree().size([height - 400, width - 160]);
		const root = d3.hierarchy(mcts_result);
		
		const link = g.append("g")
    		.attr("fill", "none")
    		.attr("stroke", "#555")
    		.attr("stroke-opacity", 0.4)
    		.attr("stroke-width", 1.5)
  			.selectAll("path")
    		.data(root.links())
    		.join("path")
      		.attr("d", d3.linkHorizontal()
          		.x(d => d.y)
          		.y(d => d.x));
  
  		const node = g.append("g")
      		.attr("stroke-linejoin", "round")
      		.attr("stroke-width", 3)
    		.selectAll("g")
    		.data(root.descendants())
    		.join("g")
      		.attr("transform", d => `translate(${d.y},${d.x})`);

  		node.append("circle")
      		.attr("fill", d => d.children ? "#555" : "#999")
      		.attr("r", 2.5);

  		node.append("text")
      		.attr("dy", "0.31em")
      		.attr("x", d => d.children ? -6 : 6)
      		.attr("text-anchor", d => d.children ? "end" : "start")
      		.text(d => d.data.name)
    		.clone(true).lower()
      		.attr("stroke", "white");
    }

    function disableScreen() {
	    var div= document.createElement("div");
	    div.className += "overlay";
	    document.body.appendChild(div);
	}

    function initalize_area() {
		//for(var i=0; i<19; i++) {
		for(var i=0; i<10; i++) {
			horizontaldraw(i);
			verticaldraw(i);
			context.strokeStyle = "#6D6E70";
			context.stroke();
		}
	}

	function horizontaldraw(i) {
		context.moveTo(10 + 20 * i, 10);
		context.lineTo(10 + 20 * i, 190);
		//context.moveTo(20 + 40 * i, 20);
		//context.lineTo(20 + 40 * i, 740);
	}


	function verticaldraw(i) {
		context.moveTo(10, 10 + 20 * i);
		context.lineTo(190, 10 + 20 * i);
		//context.moveTo(20, 20 + 40 * i);
		//context.lineTo(740, 20 + 40 * i);
	}

	function drawnodes(i, j, user) {
		var canvas = document.getElementById('gameboard');
		var context = canvas.getContext('2d');
		context.beginPath();
		context.arc(10 + 20 * i, 10 + 20 * j, 10, 0, 2 * Math.PI);
		//context.arc(20 + 40 * i, 20 + 40 * j, 10, 0, 2 * Math.PI);
		context.closePath();
		var gradient = context.createRadialGradient(10 + 20 * i, 10 + 20 * j, 0, 10 + 20 * i, 10 + 20 * j, 12)
		//var gradient = context.createRadialGradient(20 + 40 * i, 20 + 40 * j, 0, 20 + 40 * i, 20 + 40 * j, 12)
		if (user == 1) {
			gradient.addColorStop(0, 'black');
			gradient.addColorStop(1, 'black');
		} else {
			gradient.addColorStop(0, 'white');
			gradient.addColorStop(1, 'white');
		}
		context.fillStyle = gradient;
		context.fill();
	}

	canvas.onclick = function(event) {
		if(!chance) {return;} // when chance = false then it just returns.

		var x = event.offsetX;
		var y = event.offsetY;
		
		var i = Math.floor(x / 20);
		var j = Math.floor(y / 20);
		
		//var i = Math.floor(x / 40);
		//var j = Math.floor(y / 40);
			
		if (gameboard[i][j] == 0) {
			// send it to server
			var data = {
				packet: {
					cmd: 1002,
					move: { row: i, column: j }
				}				
			};
			connection.send(JSON.stringify(data));
			chance = false;
			drawnodes(i, j, 0);
			document.getElementById("status").innerHTML = "Not your turn.";
		}
	};
};
