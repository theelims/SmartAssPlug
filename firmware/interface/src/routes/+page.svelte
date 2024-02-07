<script lang="ts">
	import type { PageData } from './$types';
	import { notifications } from '$lib/components/toasts/notifications';
	import { user } from '$lib/stores/user';
	import { page } from '$app/stores';
	import { control } from '$lib/stores/control';
	import { analytics } from '$lib/stores/analytics';
	import { Chart, registerables } from 'chart.js';
	import * as LuxonAdapter from 'chartjs-adapter-luxon';
	import ChartStreaming from '@robloche/chartjs-plugin-streaming';
	import { onMount, onDestroy } from 'svelte';
	import Download from '~icons/tabler/download';
	import Start from '~icons/tabler/player-play';
	import Stop from '~icons/tabler/player-stop';
	import { decode } from 'cbor-x/decode';
	import { daisyColor } from '$lib/DaisyUiHelper';

	export let data: PageData;

	Chart.register(...registerables);
	Chart.register(LuxonAdapter);
	Chart.register(ChartStreaming);

	let pressureChartElement: HTMLCanvasElement;
	let pressureChart: Chart;

	let arousalChartElement: HTMLCanvasElement;
	let arousalChart: Chart;

	let dataSocket: WebSocket;
	let unresponsiveTimeoutData: number;
	let timeSync: number = 0;
	let lastPressure: number = 1024;
	let lastdpdt: number = 0;

	function openDataSocket() {
		dataSocket = new WebSocket('ws://' + $page.url.host + '/ws/rawData');
		dataSocket.binaryType = 'arraybuffer';
		console.log(`trying to connect to: ${dataSocket.url}`);

		dataSocket.onopen = () => {
			console.log(`connection open to: ${dataSocket.url}`);
		};

		dataSocket.onclose = () => {
			console.log(`connection closed to: ${dataSocket.url}`);
		};

		dataSocket.onmessage = (event) => {
			// Reset a timer to detect unresponsiveness
			clearTimeout(unresponsiveTimeoutData);
			unresponsiveTimeoutData = setTimeout(() => {
				console.log('Server is unresponsive');
				dataSocket.close();
			}, 5000); // Detect unresponsiveness after 1 seconds

			const data = decode(new Uint8Array(event.data));

			if (timeSync == 0) {
				timeSync = Date.now() - data[0][0];
			}

			for (let i = 0; i < data.length; i++) {
				let dpdt = (data[i][2] * 0.01 - lastPressure) * 40;
				let d2pdt2 = lastdpdt - dpdt;
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
					y: dpdt
				});
				pressureChart.data.datasets[3].data.push({
					x: timeSync + data[i][0],
					y: d2pdt2
				});
				lastPressure = data[i][2] * 0.01;
				lastdpdt = dpdt;
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

		dataSocket.onerror = () => {
			console.log(`connection error with: ${dataSocket.url}`);
		};
	}

	onDestroy(() => {
		dataSocket.close();
	});

	onMount(() => {
		openDataSocket();
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
						max: 1100,
						grid: { color: daisyColor('--bc', 10) },
						ticks: {
							stepSize: 25,
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
						//suggestedMin: -1500,
						//suggestedMax: 1500,
						min: -750,
						max: 750,
						ticks: {
							stepSize: 250,
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
						//suggestedMin: -1500,
						//suggestedMax: 1500,
						min: -750,
						max: 750,
						ticks: {
							stepSize: 250,
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
			csv += `${pressureChart.data.datasets[2].data[index].y},`;
			csv += `${pressureChart.data.datasets[3].data[index].y}\n`;
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

	function controlSession() {
		if ($control.active === true) {
			$control.active = false;
		} else {
			$control.active = true;
		}
		control.send();
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
	<div class="mt-4 mx-4">
		{#if true}
			<input
				type="range"
				min="0"
				max="100"
				bind:value={$control.value}
				on:change={() => {
					control.send();
				}}
				class="range range-primary range-xs"
			/>
			<label class="label mt-0 pt-0">
				<span class="label-text">Stimulation</span>
				<span class="label-text-alt">{$control.value} %</span>
			</label>
		{/if}
	</div>
	<div class="mt-4 mx-4">
		{#if true}
			<input
				type="range"
				min="0"
				max="100"
				bind:value={$control.vibrate}
				on:change={() => {
					control.send();
				}}
				class="range range-primary range-xs"
			/>
			<label class="label mt-0 pt-0">
				<span class="label-text">Vibrator</span>
				<span class="label-text-alt">{$control.vibrate} %</span>
			</label>
		{/if}
	</div>

	<div class="m-4 flex flex-wrap gap-6 justify-between">
		<div class="flex flex-nowrap justify-start gap-6">
			<button class="btn btn-primary inline-flex items-center w-32" on:click={controlSession}>
				{#if $control.active === false}
					<Start class="mr-2 h-5 w-5" /><span>Start</span>
				{:else}
					<Stop class="mr-2 h-5 w-5" /><span>Stop</span>
				{/if}
			</button>
			<select class="select select-primary w-44">
				<option>Simple Classifier</option>
				<option>Onwrikbaar</option>
				<option>Nogasm</option>
				<option>AI Training</option>
			</select>
		</div>

		<div class="flex flex-nowrap justify-end">
			{#if $control.active === true}
				<div class="max-h-min">
					<progress
						class="progress progress-primary w-min"
						value={$analytics.fs_percentage[$analytics.fs_percentage.length - 1]}
						max="100"
					></progress>
					<label class="label mt-0 pt-0">
						<span class="label-text">Recording</span>
						<span class="label-text-alt"
							>{$analytics.fs_percentage[$analytics.fs_percentage.length - 1].toFixed(0)} %</span
						>
					</label>
				</div>
			{:else}
				<a
					class="btn btn-primary inline-flex items-center"
					href={'http://' + $page.url.host + '/rawdata/datalog.dat'}
					download><Download class="mr-2 h-5 w-5" /><span>Download</span></a
				>
			{/if}
		</div>
	</div>
</div>
