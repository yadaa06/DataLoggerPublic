let myChart;
const ctx = document.getElementById('sensorChart').getContext('2d');
const loader = document.getElementById('loader');
const readNowBtn = document.getElementById('readNowButton');

const toggleLcd = document.getElementById('lcdToggle');
const toggleSpeaker = document.getElementById('speakerToggle');

const ws = new WebSocket(`ws://${window.location.hostname}/ws`);

ws.onopen = () => {
    console.log('WebSocket connected');
};

ws.onclose = () => {
    console.log('WebSocket disconnected');
};

ws.onmessage = (event) => {
    const state = JSON.parse(event.data);
    console.log('Received state update:', state);

    if (state.lcd_on !== undefined) {
        toggleLcd.checked = state.lcd_on;
    }
    if (state.speaker_on !== undefined) {
        toggleSpeaker.checked = state.speaker_on;
    }
};

async function togglePower(endpoint) {
    try {
        const response = await fetch(endpoint, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' }
        });
        if (!response.ok) {
            console.error('Failed to toggle power:', response.statusText);
        }
    } catch (error) {
        console.error('Network error:', error);
    }
}

async function initializeChart() {
    try {
        const response = await fetch('/dht_history');
        const data = await response.json();

        const labels = data.history.map(d => new Date(d.timestamp * 1000).toLocaleTimeString());
        const tempData = data.history.map(d => d.temperature);
        const humidityData = data.history.map(d => d.humidity);

        myChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: labels,
                datasets: [{
                    label: 'Temperature (°F)',
                    data: tempData,
                    borderColor: 'rgba(255, 99, 132, 1)',
                    yAxisID: 'y'
                }, {
                    label: 'Humidity (%)',
                    data: humidityData,
                    borderColor: 'rgba(54, 162, 235, 1)',
                    yAxisID: 'y1'
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    x: { title: { display: true, text: 'Time' } },
                    y: {
                        type: 'linear',
                        suggestedMin: 0,
                        suggestedMax: 100,
                        display: true,
                        position: 'left',
                        title: { display: true, text: 'Temperature (°F)' }
                    },
                    y1: {
                        type: 'linear',
                        suggestedMin: 0,
                        suggestedMax: 100,
                        display: true,
                        position: 'right',
                        title: { display: true, text: 'Humidity (%)' },
                        grid: { drawOnChartArea: false }
                    }
                }
            }
        });
    } catch (error) {
        console.error("Error initializing chart:", error);
    }
}

async function updateDHTdata() {
    loader.classList.remove('hidden');
    loader.classList.add('visible');
    readNowBtn.disabled = true;
    try {
        const response = await fetch('/dht_data');
        const data = await response.json();

        document.getElementById('temperature').textContent = data.temperature.toFixed(2);
        document.getElementById('humidity').textContent = data.humidity.toFixed(1);

        const now = new Date();
        const newLabel = now.toLocaleTimeString();
        document.getElementById('lastupdated').textContent = newLabel;

        console.log("Data updated successfully:", data);

        if (myChart) {
            myChart.data.labels.push(newLabel);
            myChart.data.datasets[0].data.push(data.temperature);
            myChart.data.datasets[1].data.push(data.humidity);

            if (myChart.data.labels.length > 60) {
                myChart.data.labels.shift();
                myChart.data.datasets.forEach(dataset => dataset.data.shift());
            }

            myChart.update();
        }

    } catch (error) {
        console.error("Error fetching DHT data:", error);
        document.getElementById('temperature').textContent = "Error";
        document.getElementById('humidity').textContent = "Error";
        document.getElementById('lastupdated').textContent = "Error";
    } finally {
        loader.classList.remove('visible');
        loader.classList.add('hidden');
        readNowBtn.disabled = false;
    }
}

async function fetchInitialState() {
    try {
        const response = await fetch('/status');
        const data = await response.json();
        
        if (data.lcd_on !== undefined) {
            toggleLcd.checked = data.lcd_on;
        }
        if (data.speaker_on !== undefined) {
            toggleSpeaker.checked = data.speaker_on;
        }
    } catch (error) {
        console.error("Error fetching initial state:", error);
    }
}

if (readNowBtn) {
    readNowBtn.addEventListener('click', () => {
        console.log("Read Now Button Clicked");
        updateDHTdata();
    });
}
if (toggleLcd) {
    toggleLcd.addEventListener('change', () => {
        togglePower('/lcd_toggle');
    });
}
if (toggleSpeaker) {
    toggleSpeaker.addEventListener('change', () => {
        togglePower('/speaker_toggle');
    });
}

document.addEventListener('DOMContentLoaded', () => {
    fetchInitialState();
    initializeChart();
    updateDHTdata();
});
setInterval(updateDHTdata, 60000);