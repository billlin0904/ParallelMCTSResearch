var usr = 1; // means black color
var chance = true;

// inintalize whole board
var gameboard = [];

var HAS_CHART = false;
var MAX_WIDTH = 10;
var MAX_HEIGHT = 10;

var IS_WATCH = true;

var rectW = 60;
var rectH = 30;
var duration = 750;

var margin = {
        top: 20,
        right: 120,
        bottom: 20,
        left: 120
    },

    width = 960 - margin.right - margin.left,
    height = 800 - margin.top - margin.bottom;

clearBoard();

function clearBoard() {
    for (var i = 0; i < MAX_WIDTH; i++) {
        gameboard[i] = [];
        for (var j = 0; j < MAX_HEIGHT; j++) {
            gameboard[i][j] = 0;
        }
    }
}

var root = {
    "name": "root",
    "children": []
};

window.onload = function() {
    var canvas = document.getElementById("gameboard");
    var context = canvas.getContext('2d');
    var connection = new WebSocket('ws://127.0.0.1:9090');
    //var connection = new WebSocket('ws://10.28.24.103:9090');

    connection.onopen = function() {
        connection.send(JSON.stringify({
            packet: {
                cmd: 1000,
				is_watch: IS_WATCH,
            }
        }));
        initalize_area();
    };

    connection.onerror = function(error) {
        console.log('WebSocket Error ' + error);
    };

    connection.onmessage = function(message) {
        try {
            var json = JSON.parse(message.data);
        } catch (e) {
            console.log(e);
            return;
        }

        if (json.packet.cmd == 1002) {
            var i = json.packet.move.row;
            var j = json.packet.move.column;
            var color = json.packet.player_id;
			if (color == 1) {
				root = json.mcts_result;
				if (HAS_CHART) {
					update(root);
				}
			}            
            gameboard[i][j] = color;         
			drawnodes(i, j, color);
            if (json.color != usr) {
                chance = true;
            }
        } else if (json.packet.cmd == 1001) {
            clearBoard();
            var canvas = document.getElementById('gameboard');
            var context = canvas.getContext('2d');
            context.clearRect(0, 0, canvas.width, canvas.height);
            chance = true;
            initalize_area();
        }
    };

    var i = 0,
        duration = 750,
        rectW = 60,
        rectH = 30;

    var tree = d3.layout.tree().nodeSize([70, 40]);
    var diagonal = d3.svg.diagonal()
        .projection(function(d) {
            return [d.x + rectW / 2, d.y + rectH / 2];
        });

    var svg = d3.select("#body").append("svg").attr("width", 2000).attr("height", 1000)
        .call(zm = d3.behavior.zoom().scaleExtent([1, 3]).on("zoom", redraw)).append("g")
        .attr("transform", "translate(" + 350 + "," + 20 + ")");

    //necessary so that zoom knows where to zoom and unzoom from
    zm.translate([350, 20]);
	
    function collapse(d) {
        if (d.children) {
            d._children = d.children;
            d._children.forEach(collapse);
            d.children = null;
        }
    }

    root.children.forEach(collapse);

    d3.select("#body").style("height", "800px");

    function roundTo(n, digits) {
        var negative = false;
        if (digits === undefined) {
            digits = 0;
        }
        if (n < 0) {
            negative = true;
            n = n * -1;
        }
        var multiplicator = Math.pow(10, digits);
        n = parseFloat((n * multiplicator).toFixed(11));
        n = (Math.round(n) / multiplicator).toFixed(2);
        if (negative) {
            n = (n * -1).toFixed(2);
        }
        return n;
    }

    String.prototype.format = function(args) {
        var result = this;
        if (arguments.length > 0) {
            if (arguments.length == 1 && typeof(args) == "object") {
                for (var key in args) {
                    if (args[key] != undefined) {
                        var reg = new RegExp("({" + key + "})", "g");
                        result = result.replace(reg, args[key]);
                    }
                }
            } else {
                for (var i = 0; i < arguments.length; i++) {
                    if (arguments[i] != undefined) {
                        var reg = new RegExp("({)" + i + "(})", "g");
                        result = result.replace(reg, arguments[i]);
                    }
                }
            }
        }
        return result;
    }

    function update(source) {
        root.x0 = 0;
        root.y0 = height / 2;

        // Compute the new tree layout.
        var nodes = tree.nodes(root).reverse(),
            links = tree.links(nodes);

        // Normalize for fixed-depth.
        nodes.forEach(function(d) {
            d.y = d.depth * 180;
        });

        // Update the nodes…
        var node = svg.selectAll("g.node")
            .data(nodes, function(d) {
                return d.id || (d.id = ++i);
            });

        // Enter any new nodes at the parent's previous position.
        var nodeEnter = node.enter().append("g")
            .attr("class", "node")
            .attr("transform", function(d) {
                return "translate(" + source.x0 + "," + source.y0 + ")";
            })
            .on("click", click);

        nodeEnter.append("rect")
            .attr("width", rectW)
            .attr("height", rectH)
            .attr("stroke", "black")
            .attr("stroke-width", 1)
            .style("fill", function(d) {
                return d._children ? "lightsteelblue" : "#fff";
            });

        nodeEnter.append("text")
            .attr("x", rectW / 2)
            .attr("y", rectH / 2)
            .attr("dy", ".35em")
            .attr("text-anchor", "middle")
            .text(function(d) {
                var s = "[" + d.name + "] {0}";
                return s.format(roundTo(d.value, 1));
            });

        // Transition nodes to their new position.
        var nodeUpdate = node.transition()
            .duration(duration)
            .attr("transform", function(d) {
                return "translate(" + d.x + "," + d.y + ")";
            });

        nodeUpdate.select("rect")
            .attr("width", rectW)
            .attr("height", rectH)
            .attr("stroke", "black")
            .attr("stroke-width", 1)
            .style("fill", function(d) {
                return d._children ? "lightsteelblue" : "#fff";
            });

        nodeUpdate.select("text")
            .style("fill-opacity", 1);

        // Transition exiting nodes to the parent's new position.
        var nodeExit = node.exit().transition()
            .duration(duration)
            .attr("transform", function(d) {
                return "translate(" + source.x + "," + source.y + ")";
            })
            .remove();

        nodeExit.select("rect")
            .attr("width", rectW)
            .attr("height", rectH)
            .attr("stroke", "black")
            .attr("stroke-width", 1);

        nodeExit.select("text");

        // Update the links…
        var link = svg.selectAll("path.link")
            .data(links, function(d) {
                return d.target.id;
            });

        // Enter any new links at the parent's previous position.
        link.enter().insert("path", "g")
            .attr("class", "link")
            .attr("x", rectW / 2)
            .attr("y", rectH / 2)
            .attr("d", function(d) {
                var o = {
                    x: source.x0,
                    y: source.y0
                };
                return diagonal({
                    source: o,
                    target: o
                });
            });

        // Transition links to their new position.
        link.transition()
            .duration(duration)
            .attr("d", diagonal);

        // Transition exiting nodes to the parent's new position.
        link.exit().transition()
            .duration(duration)
            .attr("d", function(d) {
                var o = {
                    x: source.x,
                    y: source.y
                };
                return diagonal({
                    source: o,
                    target: o
                });
            })
            .remove();

        // Stash the old positions for transition.
        nodes.forEach(function(d) {
            d.x0 = d.x;
            d.y0 = d.y;
        });
    }

    // Toggle children on click.
    function click(d) {
        if (d.children) {
            d._children = d.children;
            d.children = null;
        } else {
            d.children = d._children;
            d._children = null;
        }
        update(d);
    }

    //Redraw for zoom
    function redraw() {
        //console.log("here", d3.event.translate, d3.event.scale);
        svg.attr("transform",
            "translate(" + d3.event.translate + ")" +
            " scale(" + d3.event.scale + ")");
    }

    function disableScreen() {
        var div = document.createElement("div");
        div.className += "overlay";
        document.body.appendChild(div);
    }

    function initalize_area() {
        for (var i = 0; i < 10; i++) {
            context.beginPath();
            horizontaldraw(i);
            verticaldraw(i);
            context.strokeStyle = "#6D6E70";
            context.stroke();
        }
    }

    function horizontaldraw(i) {
        context.moveTo(10 + 20 * i, 10);
        context.lineTo(10 + 20 * i, 190);
    }

    function verticaldraw(i) {
        context.moveTo(10, 10 + 20 * i);
        context.lineTo(190, 10 + 20 * i);
    }

    function drawnodes(i, j, user) {
        var canvas = document.getElementById('gameboard');
        var context = canvas.getContext('2d');
        context.beginPath();
        context.arc(10 + 20 * i, 10 + 20 * j, 10, 0, 2 * Math.PI);
        var gradient = context.createRadialGradient(10 + 20 * i, 10 + 20 * j, 0, 10 + 20 * i, 10 + 20 * j, 12);
        context.closePath();
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
        if (!chance) {
            return;
        } // when chance = false then it just returns.

        var x = event.offsetX;
        var y = event.offsetY;

        var i = Math.floor(x / 20);
        var j = Math.floor(y / 20);

        if (gameboard[i][j] == 0) {
            // send it to server
            var data = {
                packet: {
                    cmd: 1002,
                    move: {
                        row: i,
                        column: j
                    }
                }
            };
            chance = false;
            drawnodes(i, j, 0);
			if (!IS_WATCH) {
				connection.send(JSON.stringify(data));
			}            
			root.children.forEach(collapse);
        }
    };
};