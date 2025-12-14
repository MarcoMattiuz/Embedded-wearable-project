var traceFILTERED = {
  type: "scatter",
  line: { width: 2, color: "#4ecdc4" },
};
var traceRAW = {
  type: "scatter",
  line: { width: 2, color: "#ff9a56" },
};
var traceBPM = {
  type: "scatter",
  mode: "lines+markers",
  name: "BPM",
  line: { width: 2, color: "#ff6b35" },
  xaxis: "x",
  yaxis: "y",
  hovertemplate: "BPM: %{y}<br>Time: %{customdata}<extra></extra>",
  customdata: [],
};
var traceAVGBPM = {
  type: "scatter",
  mode: "lines+markers",
  name: "AVG BPM",
  xaxis: "x2",
  yaxis: "y2",
  line: { width: 2, color: "#4ecdc4" },
  hovertemplate: "AVG BPM: %{y}<br>Time: %{customdata}<extra></extra>",
  customdata: [],
};

var layoutFILTERED = {
  autosize: true,
  height: 300,
  title: { text: "FILTERED signal", font: { color: "white" } },
  margin: { l: 60, r: 20, t: 40, b: 60 },
  paper_bgcolor: "rgba(0,0,0,0)",
  plot_bgcolor: "rgba(0,0,0,0)",
  xaxis: {
    title: "Time (s)",
    titlefont: { color: "white" },
    tickfont: { color: "white" },
    showgrid: false,
    zeroline: true,
  },
  yaxis: {
    title: "IR_AC",
    titlefont: { color: "white" },
    tickfont: { color: "white" },
    showgrid: false,
    zeroline: true,
  },
};
var layoutRAW = {
  autosize: true,
  height: 300,
  title: { text: "RAW signal", font: { color: "white" } },
  margin: { l: 60, r: 20, t: 40, b: 60 },
  paper_bgcolor: "rgba(0,0,0,0)",
  plot_bgcolor: "rgba(0,0,0,0)",
  xaxis: {
    title: "Time (s)",
    titlefont: { color: "white" },
    tickfont: { color: "white" },
    showgrid: false,
    zeroline: true,
  },
  yaxis: {
    title: "IR_AC",
    titlefont: { color: "white" },
    tickfont: { color: "white" },
    showgrid: false,
    zeroline: true,
  },
};
var layoutBPM = {
  autosize: true,
  height: 300,
  title: { text: "BPM & AVG_BPM", font: { color: "white" } },
  margin: { l: 60, r: 20, t: 40, b: 60 },
  paper_bgcolor: "rgba(0,0,0,0)",
  plot_bgcolor: "rgba(0,0,0,0)",
  xaxis: {
    title: "Time (s)",
    titlefont: { color: "white" },
    tickfont: { color: "white" },
    showgrid: false,
    zeroline: true,
  },
  yaxis: {
    title: "IR_AC",
    titlefont: { color: "white" },
    tickfont: { color: "white" },
    showgrid: false,
    zeroline: true,
  },
  xaxis2: {
    title: "Time (s)",
    titlefont: { color: "white" },
    tickfont: { color: "white" },
    showgrid: false,
    zeroline: true,
    overlaying: "x",
    side: "top",
  },
  yaxis2: {
    title: "IR_AC",
    titlefont: { color: "white" },
    tickfont: { color: "white" },
    showgrid: false,
    zeroline: true,
    overlaying: "y",
    side: "right",
  },
};

var config = {
  staticPlot: true,
  displayModeBar: false,
  responsive: true,
};
var config2 = {
  staticPlot: false,
  displayModeBar: false,
  responsize: true,
};

Plotly.newPlot("graphFILTERED", [traceFILTERED], layoutFILTERED, config);
Plotly.newPlot("graphRAW", [traceRAW], layoutRAW, config);
Plotly.newPlot("graphBPM", [traceBPM, traceAVGBPM], layoutBPM, config2);

function updateBPMGraph() {
  if (
    !window.BPMsampleArr ||
    window.BPMsampleArr.length === 0 ||
    !window.AVGBPMsampleArr ||
    window.AVGBPMsampleArr.length === 0
  )
    return;

  const BPMarr = window.BPMsampleArr.map(item => item.value);
  const BPMsamples = BPMarr.map((_, i) => i);
  const BPMtime = BPMsamples.map((s) => s * 2.5);
  const BPMtimestamps = window.BPMsampleArr.map(item => item.timestamp);

  const AVGBPMarr = window.AVGBPMsampleArr.map(item => item.value);
  const AVGBPMsamples = AVGBPMarr.map((_, i) => i);
  const AVGBPMtime = AVGBPMsamples.map((s) => s * 2.5);
  const AVGBPMtimestamps = window.AVGBPMsampleArr.map(item => item.timestamp);

  Plotly.update("graphBPM", {
    x: [BPMtime, AVGBPMtime],
    y: [BPMarr, AVGBPMarr],
    customdata: [BPMtimestamps, AVGBPMtimestamps],
  });
}
function updateIRACGraphs() {
  if (!window.IRACsampleArr || window.IRACsampleArr.length === 0) return;

  const arr = window.IRACsampleArr;
  const samples = arr.map((_, i) => i);
  const time = samples.map((s) => s * 0.02);
  const N = 240;
  const start = Math.max(0, time.length - N);
  const end = time.length;
  const arr_max = Math.max(arr);
  const arr_min = Math.min(arr);

  Plotly.update(
    "graphFILTERED",
    {
      x: [time],
      y: [arr],
    },
    {
      xaxis: {
        range: [time[start], time[end - 1]],
      },
    },
    {
      yaxis: {
        range: [arr[arr_max], arr[arr_min]],
      },
    }
  );
}

function updateIRRAWGraphs() {
  if (!window.IRRAWsampleArr || window.IRRAWsampleArr.length === 0) return;

  const arr = window.IRRAWsampleArr;
  const samples = arr.map((_, i) => i);
  const time = samples.map((s) => s * 0.02);
  const N = 240;
  const start = Math.max(0, time.length - N);
  const end = time.length;
  const arr_max = Math.max(arr);
  const arr_min = Math.min(arr);

  
  Plotly.update(
    "graphRAW",
    {
      x: [time],
      y: [arr],
    }
  );
}
