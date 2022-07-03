function gistogram()
{
    let object = {

        options : {
            min_x : 0,
            max_x : 300,
            width : 300,
            height : 100,
            dimensions : 0
        },

        rboffset : 0.0, // right wall
        lboffset : 0.0, // left wall

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
            { min:  180, max: 300, func: (x, h) => `rgba(  225,       225,${255 - x - 60},1)` },
        ],

        data: [
            // { x : 1, y : 10 },
        ],

        update: (options) => { object.options = options },

        
        set_left_bound : offset => {

            offset = Math.min(offset, 1.0 - object.rboffset);
            offset = Math.max(offset, 0.0);

            object.lboffset = offset;

        },

        set_right_bound : offset => {

            offset = Math.min(offset, 1.0 - object.lboffset);
            offset = Math.max(offset, 0.0);

            object.rboffset = offset;
        },

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
                        // context.fillRect(x, y, w, h);
                        context.fillRect(x, height - y, w, -h);
        
                        context.closePath()
                    }   
                });

                
                context.beginPath()
                    
                context.fillStyle = 'rgba(0,0,0,.5)';

                context.fillRect(0, 0, width * object.lboffset, height);
                context.fillRect(width, 0, -width * object.rboffset, height);

                context.closePath()
            }
        }
    };

    return object;
}

let t = gistogram();

var cnv = document.createElement('canvas');

cnv.style = "width:fit-content;"

for(let x = 0; x < 300; ++x)
{
    t.data.push({x, y : x});
}

t.set_left_bound(0.5)
t.draw(cnv)

document.body.appendChild(cnv)
