"use strict";
var output;
var ws;
var Module;
var canvas;
var ctx;
var input;
var img;


function init() {
    output = document.getElementById("output");
    input  = document.getElementById('input');
	canvas = document.getElementById("canvas");
	ctx    = canvas.getContext('2d');
		
    if (Module)
    {
        Module.setReceiveData(onReceiveData);
    }
}

function writeToScreen(message) {
    let p = document.createElement('p');
    p.style.wordWrap = 'break-word';
    p.innerHTML = message;

    if (output.childNodes.length > 5) {
        output.removeChild(output.childNodes[0]);
    }
    output.appendChild(p);
}

function onReceiveData(message)
{
    let command = message.command;
    let content = message.content;

    switch(command)
    {
        case "onBinary": 
        {
            img = new Image();

            img.onload = () => {
                ctx.drawImage(img, 0, 0);
            };

            const promise = new Promise((resolve, reject) => {
                resolve(new Blob([new Uint8Array(content).buffer], { type : 'image/bmp' }))
            });

            promise.then(blob => img.src = URL.createObjectURL(blob));
        }
        break;

        case "onString":
        {
            writeToScreen('<span style="color: blue;">RESPONSE: ' + content + '</span>');
        }
        break;
    }
}

function doSend(message) {
    writeToScreen("SENT: " + message);

    Module.sendData({command : "onString", content : message })
}

window.addEventListener('load', init, false);

function wsSend() {
    doSend(document.getElementById('data_to_send').value);
}

function wsSendBinary() {
    var array = new Uint8Array(5);

    for (var i = 0; i < array.length; ++i) {
        array[i] = i + 5;
    }

    Module.sendData({command : 'onBinary', content : array });
}

function gistogram()
{
    let object = {

        options : {
            min_x : 0,
            max_x : 389,
            width : 389,
            height : 100,
            dimensions : 0
        },

        colorset_verify: () => {

            const length = object.colorset.length;

            if (length > 1)
            {
                for(let index = 0; index < length - 1; ++index)
                {
                    const prev = object.colorset[index + 0];
                    const next = object.colorset[index + 1];

                    if (prev.max != next.min || prev.func(prev.max, 0) != next.func(next.min, 0))
                    {
                        return false;
                    }
                }
            }

            return length > 0;
        },

        colorset : [

            { min:   0, max:  90, func: (x, h) => `rgba(${255 - x},        0,        0,1)` },
            { min:  90, max:  180, func: (x, h) => `rgba( 225,${255 - x - 30},        0,1)` },
            { min:  180, max: 389, func: (x, h) => `rgba(  225,       225,${255 - x - 60},1)` },
        ],

        data: [
            // { x : 1, y : 10 },
        ],

        update: (options) => { object.options = options },

        clear: (canvas /*: HTMLCanvasElement*/) => {

            let context = canvas.getContext('2d');

            if (context)
            {
                context.clearRect(0, 0, object.options.width, object.options.height);
            }
        },

        draw: (canvas /*: HTMLCanvasElement*/) => {

            canvas.width = object.options.width;
            canvas.height = object.options.height;

            object.clear(canvas);
            object.render(canvas);
        },

        render : (canvas /*: HTMLCanvasElement*/) => {

            let context = canvas.getContext('2d');

            if(context)
            {
                let __max_y = object.data[0].y
                let __min_y = object.data[0].y

                object.data.forEach( dt => {
                    if (dt.y > __max_y)
                    {
                        __max_y = dt.y
                    }

                    if (dt.y < __min_y)
                    {
                        __min_y = dt.y
                    }
                });

                const min_y = __min_y;
                const max_y = __max_y;
                const min_x = object.options.min_x;
                const max_x = object.options.max_x;

                const width = object.options.width;
                const height = object.options.height;

                const length = object.data.length;
                const dims = Math.max(width, object.options.dimensions)

                const scale_x = width / (max_x - min_x);
                const scale_y = height / max_y;
                
                const y = 0;
                const w = width / dims;

                let index = 0;


                object.colorset.forEach(color => {
                    for(;index < length && color.min <= object.data[index].x && object.data[index].x < color.max; ++index)
                    {
                        const x = scale_x * (object.data[index].x - min_x);
                        const h = scale_y * (object.data[index].y);

                        //console.log(object.data[index].x)

                        context.beginPath()
                    
                        context.fillStyle = color.func(x,h);
                        
                        context.fillRect(x, height - y, w, -h);

                        context.fillStyle = 'black';
                        context.fillRect(x, height - h, w, 1);
        
                        context.closePath()
                    }   
                });
            }
        }
    };

    return object;
}

var t = gistogram();

var cnv = document.querySelector('.gistogramm');

// cnv.style = "width:fit-content;"

for(let x = 0; x < 389; ++x)
{
    t.data.push({x, y : 100 * Math.exp(-x / 100) });
}

t.draw(cnv)

// document.body.appendChild(cnv)
