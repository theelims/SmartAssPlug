<script lang="ts">
	import type { PageData } from './$types';
	import { notifications } from '$lib/components/toasts/notifications';
	import { user } from '$lib/stores/user';
	import { page } from '$app/stores';
	import { telemetry } from '$lib/stores/telemetry';
	import { Chart, registerables } from 'chart.js';
	import * as LuxonAdapter from 'chartjs-adapter-luxon';
	import ChartStreaming from '@robloche/chartjs-plugin-streaming';
	import { onMount, onDestroy } from 'svelte';
	import Download from '~icons/tabler/download';
	import Start from '~icons/tabler/player-record';
	import Stop from '~icons/tabler/player-stop';
	import { decode } from 'cbor-x/decode';

	export let data: PageData;

	Chart.register(...registerables);
	Chart.register(LuxonAdapter);
	Chart.register(ChartStreaming);

	let pressureChartElement: HTMLCanvasElement;
	let pressureChart: Chart;

	let arousalChartElement: HTMLCanvasElement;
	let arousalChart: Chart;

	let edgingSocket: WebSocket;
	let reconnectWSIntervalId: number = 0;
	let unresponsiveTimeout: number;
	let timeSync: number = 0;

	function openSocket() {
		edgingSocket = new WebSocket('ws://' + $page.url.host + '/ws/rawData');
		edgingSocket.binaryType = 'arraybuffer';
		console.log(`trying to connect to: ${edgingSocket.url}`);

		edgingSocket.onopen = () => {
			console.log(`connection open to: ${edgingSocket.url}`);

			timeSync = 0;
			clearInterval(reconnectWSIntervalId);
			reconnectWSIntervalId = 0;

			edgingSocket.onclose = () => {
				console.log(`connection closed to: ${edgingSocket.url}`);
				reconnect();
			};

			edgingSocket.onmessage = (event) => {
				// Reset a timer to detect unresponsiveness
				clearTimeout(unresponsiveTimeout);
				unresponsiveTimeout = setTimeout(() => {
					console.log('Server is unresponsive');
					reconnect();
				}, 1000); // Detect unresponsiveness after 1 seconds

				const data = decode(new Uint8Array(event.data));

				if (timeSync == 0) {
					timeSync = Date.now() - data[0][0];
				}

				for (let i = 0; i < data.length; i++) {
					pressureChart.data.datasets[0].data.push({
						x: timeSync + data[i][0],
						y: data[i][1] * 0.01
					});
					pressureChart.data.datasets[1].data.push({
						x: timeSync + data[i][0],
						y: data[i][2] * 0.01
					});
					pressureChart.data.datasets[2].data.push({
						x: timeSync + data[i][0],
						y: data[i][3] * 0.01
					});
					pressureChart.data.datasets[3].data.push({
						x: timeSync + data[i][0],
						y: data[i][4] * 0.01
					});
				}
				/*
					arousalChart.data.datasets[0].data.push({
						x: timeSync + data.payload.timestamp.slice(-1)[0],
						y: data.payload.arousal_state
					});
					arousalChart.data.datasets[1].data.push({
						x: timeSync + data.payload.timestamp.slice(-1)[0],
						y: data.payload.arousal_value + 50
					}); */
			};
		};

		edgingSocket.onerror = () => {
			console.log(`connection error with: ${edgingSocket.url}`);
			reconnect();
		};
	}

	function reconnect() {
		console.log('WS reconnect triggered');
		if (reconnectWSIntervalId === 0) {
			edgingSocket.close;
			reconnectWSIntervalId = setInterval(openSocket, 2000);
			console.log('WS reconnect Timer ID: ' + reconnectWSIntervalId);
		}
	}

	onDestroy(() => edgingSocket.close());

	function daisyColor(name: string, opacity: number = 100) {
		const color = getComputedStyle(document.documentElement).getPropertyValue(name);
		return 'hsla(' + color + '/ ' + Math.min(Math.max(Math.round(opacity), 0), 100) + '%)';
	}

	onMount(() => {
		openSocket();
		pressureChart = new Chart(pressureChartElement, {
			type: 'line',
			data: {
				datasets: [
					{
						// Raw Pressure
						borderColor: daisyColor('--p', 15),
						fill: false,
						pointRadius: 0,
						data: [],
						yAxisID: 'y'
					},
					{
						// Kalman filtered pressure
						borderColor: daisyColor('--pf'),
						fill: false,
						pointRadius: 0,
						data: [],
						yAxisID: 'y'
					},
					{
						// 1st derivative
						borderColor: daisyColor('--s'),
						fill: true,
						pointRadius: 0,
						data: [],
						yAxisID: 'y1'
					},
					{
						// 1st derivative
						borderColor: daisyColor('--a'),
						fill: true,
						pointRadius: 0,
						data: [],
						yAxisID: 'y2'
					}
				]
			},
			options: {
				animation: false,
				responsive: true,
				maintainAspectRatio: false,
				interaction: {
					mode: 'index',
					intersect: false
				},
				plugins: {
					// Change options for ALL axes of THIS CHART
					streaming: {
						duration: 10000,
						refresh: 25,
						delay: 100
					},
					tooltip: {
						enabled: false
					},
					legend: {
						display: false
					}
				},
				scales: {
					x: {
						type: 'realtime',
						grid: {
							color: daisyColor('--bc', 10)
						},
						ticks: { color: daisyColor('--bc') }
					},
					y: {
						type: 'linear',
						title: {
							display: true,
							text: 'Pressure [mbar]',
							color: daisyColor('--p'),
							font: {
								size: 16,
								weight: 'bold'
							}
						},
						position: 'left',
						min: 950,
						max: 1250,
						grid: { color: daisyColor('--bc', 10) },
						ticks: {
							stepSize: 50,
							color: daisyColor('--bc')
						},
						border: { color: daisyColor('--bc', 10) }
					},
					y1: {
						type: 'linear',
						title: {
							display: true,
							text: 'Δp/dt [mbar/s]',
							color: daisyColor('--s'),
							font: {
								size: 16,
								weight: 'bold'
							}
						},
						position: 'right',
						suggestedMin: -1500,
						suggestedMax: 1500,
						ticks: {
							stepSize: 500,
							color: daisyColor('--bc')
						},
						grid: {
							drawOnChartArea: false // only want the grid lines for one axis to show up
						},
						border: { color: daisyColor('--bc', 10) }
					},
					y2: {
						type: 'linear',
						title: {
							display: true,
							text: 'Δ2p/dt² [mbar/s²]',
							color: daisyColor('--a'),
							font: {
								size: 16,
								weight: 'bold'
							}
						},
						position: 'right',
						suggestedMin: -1500,
						suggestedMax: 1500,
						ticks: {
							stepSize: 500,
							color: daisyColor('--bc')
						},
						grid: {
							drawOnChartArea: false // only want the grid lines for one axis to show up
						},
						border: { color: daisyColor('--bc', 10) }
					}
				}
			}
		});
		arousalChart = new Chart(arousalChartElement, {
			type: 'line',
			data: {
				datasets: [
					{
						backgroundColor: daisyColor('--p', 10),
						borderColor: daisyColor('--p'),
						fill: false,
						pointRadius: 0,
						stepped: true,
						data: [],
						yAxisID: 'y'
					},
					{
						// Arousal Value
						borderColor: daisyColor('--s'),
						fill: true,
						pointRadius: 0,
						data: [],
						yAxisID: 'y1'
					}
				]
			},
			options: {
				animation: false,
				responsive: true,
				maintainAspectRatio: false,
				interaction: {
					mode: 'index',
					intersect: false
				},
				plugins: {
					// Change options for ALL axes of THIS CHART
					streaming: {
						duration: 60000,
						refresh: 25,
						delay: 100
					},
					tooltip: {
						enabled: false
					},
					legend: {
						display: false
					}
				},
				scales: {
					x: {
						type: 'realtime',
						grid: {
							color: daisyColor('--bc', 10)
						},
						ticks: { color: daisyColor('--bc') }
					},
					y: {
						type: 'category',
						labels: ['Orgasm', 'Edging', 'Aroused', 'Neutral'],
						title: {
							display: true,
							text: 'Arousal State',
							color: daisyColor('--p'),
							font: {
								size: 16,
								weight: 'bold'
							}
						},
						position: 'left',
						grid: { color: daisyColor('--bc', 10) },
						ticks: {
							color: daisyColor('--bc')
						},
						border: { color: daisyColor('--bc', 10) }
					},
					y1: {
						type: 'linear',
						title: {
							display: true,
							text: 'Arousal [%]',
							color: daisyColor('--s'),
							font: {
								size: 16,
								weight: 'bold'
							}
						},
						position: 'right',
						suggestedMin: 0,
						suggestedMax: 100,
						ticks: {
							stepSize: 100 / 5,
							color: daisyColor('--bc')
						},
						grid: {
							drawOnChartArea: false // only want the grid lines for one axis to show up
						},
						border: { color: daisyColor('--bc', 10) }
					},
					y2: {
						type: 'linear',
						title: {
							display: true,
							text: 'Stimulation [%]',
							color: daisyColor('--a'),
							font: {
								size: 16,
								weight: 'bold'
							}
						},
						position: 'right',
						suggestedMin: 0,
						suggestedMax: 100,
						ticks: {
							stepSize: 100 / 5,
							color: daisyColor('--bc')
						},
						grid: {
							drawOnChartArea: false // only want the grid lines for one axis to show up
						},
						border: { color: daisyColor('--bc', 10) }
					}
				}
			}
		});
	});

	function convertToCSV() {
		let csv = 't,raw_pres,smooth_pres,dpdt,d2pdt2\n'; // CSV header

		pressureChart.data.datasets[0].data.forEach((point, index) => {
			csv += `${point.x},${point.y},`; // Add each data point as a CSV row
			csv += `${pressureChart.data.datasets[1].data[index].y},`;
			csv += `${pressureChart.data.datasets[1].data[index].y},`;
			csv += `${pressureChart.data.datasets[1].data[index].y},`;
			csv += `${pressureChart.data.datasets[1].data[index].y}\n`;
		});

		return csv;
	}

	function downloadData() {
		//const dataJson = JSON.stringify(pressureChart.data, null, 2);
		//const dataJsonBlob = new Blob([dataJson], { type: 'application/json' });
		// Create a Blob URL
		//const blobURL = URL.createObjectURL(dataJsonBlob);

		const csvString = convertToCSV();

		const blob = new Blob([csvString], { type: 'text/csv' });

		const blobURL = URL.createObjectURL(blob);

		// Create a download link
		const downloadLink = document.createElement('a');
		downloadLink.href = blobURL;
		downloadLink.download = 'rawdata.txt'; // Set the desired file name

		// Simulate a click event to trigger the download
		document.body.appendChild(downloadLink);
		downloadLink.click();

		// Clean up by removing the download link and revoking the Blob URL
		document.body.removeChild(downloadLink);
		URL.revokeObjectURL(blobURL);
	}
</script>

<div class="card bg-base-200 shadow-md shadow-primary/50 mt-3 mb-1.5 mx-auto w-11/12">
	<div class="relative h-72 md:h-96 w-full p-2">
		<canvas bind:this={pressureChartElement} />
	</div>
</div>
<div class="card bg-base-200 shadow-md shadow-primary/50 my-1.5 mx-auto w-11/12">
	<div class="relative h-64 w-full p-2">
		<canvas bind:this={arousalChartElement} />
	</div>
</div>
<div class="card bg-base-200 shadow-md shadow-primary/50 my-1.5 mx-auto w-11/12">
	<div class="m-4 flex flex-wrap gap-2">
		<div class="grow"></div>
		<button class="btn btn-primary inline-flex items-center" on:click={downloadData}
			><Download class="mr-2 h-5 w-5" /><span>Download CSV</span></button
		>
	</div>
</div>
