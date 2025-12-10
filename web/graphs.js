
var trace1 = {
  type: 'scatter',
  line: { width: 2, color: 'orange'}
};

var layout = {
    title: {
        text: 'IR filtered sample'
    },
    margin: { l: 60, r: 20, t: 40, b: 60 },
    paper_bgcolor: 'rgba(0,0,0,0)', 
    plot_bgcolor: 'rgba(0,0,0,0)',
    xaxis: {
        title: 'Time (s)', 
        titlefont: { color: 'white' },
        tickfont: { color: 'white' },
        showgrid: false,   
        zeroline: true,   
    },
    yaxis: {
        title: 'IR_AC',
        titlefont: { color: 'white' },
        tickfont: { color: 'white' },
        showgrid: false,   
        zeroline: true,   
    }
};

var config = {
    staticPlot: true,
    displayModeBar: false
}
var data = [trace1];

Plotly.newPlot('graph', 
    data, 
    layout, 
    config);

//TODO: add button to stop the update of a graph
function updateGraphs() {
    if (!window.IRACsampleArr || window.IRACsampleArr.length === 0) return;

    const arr = window.IRACsampleArr;
    const samples = arr.map((_, i) => i);
    const time = samples.map(s => s * 0.02);
    console.log(arr.length);
    const N = 240; 
    const start = Math.max(0, time.length - N);
    const end = time.length;
    const arr_max = Math.max(arr);
    const arr_min = Math.min(arr);

    Plotly.update('graph', 
        {
            x: [time],
            y: [arr]
        },
        {
            xaxis: {
                range: [ time[start], time[end - 1] ]
            }
        },
        {
            yaxis:{
              range: [ arr[arr_max], arr[arr_min]]
            }
        }
    );
}
