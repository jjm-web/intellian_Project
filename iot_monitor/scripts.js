document.addEventListener('DOMContentLoaded', () => {
    async function fetchData() {
        try {
            const response = await fetch('http://172.30.1.86:5000/data', {
                mode: 'cors'
            });
            if (!response.ok) {
                throw new Error('Network response was not ok');
            }
            const data = await response.json();
            console.log('Data received:', data);
            let tableContent = '';
            data.forEach(row => {
                tableContent += `<tr>
                    <td>${row.timestamp}</td>
                    <td>${row.signal_strength.toFixed(2)}</td>
                    <td>${row.latency.toFixed(2)}</td>
                    <td>${row.status}</td>
                    <td>${row.latitude.toFixed(10)}</td>
                    <td>${row.longitude.toFixed(10)}</td>
                    <td>${row.address}</td>
                    <td>${row.gyroscope_x.toFixed(10)}</td>
                    <td>${row.gyroscope_y.toFixed(10)}</td>
                    <td>${row.gyroscope_z.toFixed(10)}</td>
                    <td>${row.temperature.toFixed(2)}</td>
                    <td>${row.humidity.toFixed(2)}</td>
                </tr>`;
            });
            document.querySelector('#data-table tbody').innerHTML = tableContent;
        } catch (error) {
            console.error('Error fetching data:', error);
        }
    }

    setInterval(fetchData, 2000);
    fetchData();
});
