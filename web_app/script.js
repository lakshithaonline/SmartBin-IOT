setInterval(() => {
    fetch('http://<Arduino_IP>/') // Replace with your Arduino IP address
        .then(response => response.json())
        .then(data => {
            document.getElementById('binStatus').textContent = data.binStatus;
            document.getElementById('weight').textContent = data.weight;
            document.getElementById('gasLevel').textContent = data.gasStatus;
        })
        .catch(error => console.error('Error fetching data:', error));
}, 1000);
