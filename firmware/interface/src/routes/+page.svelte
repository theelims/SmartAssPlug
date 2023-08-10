<script lang="ts">
	import type { PageData } from './$types';
	import { notifications } from '$lib/components/toasts/notifications';
	import { user } from '$lib/stores/user';
	import { page } from '$app/stores';
	import { Chart, registerables } from 'chart.js';
	import * as LuxonAdapter from 'chartjs-adapter-luxon';
	import ChartStreaming from '@robloche/chartjs-plugin-streaming';
	import { onMount, onDestroy } from 'svelte';

	export let data: PageData;

	Chart.register(...registerables);
	Chart.register(LuxonAdapter);
	Chart.register(ChartStreaming);

	let pressureChartElement: HTMLCanvasElement;
	let pressureChart: Chart;

	let arousalChartElement: HTMLCanvasElement;
	let arousalChart: Chart;

	const edgingSocket = new WebSocket('ws://' + $page.url.host + '/ws/edgingData');
	edgingSocket.onopen = (event) => {
		edgingSocket.send('Hello');
	};

	edgingSocket.addEventListener('close', (event) => {
		const closeCode = event.code;
		const closeReason = event.reason;
		console.log('WebSocket closed with code:', closeCode);
		console.log('Close reason:', closeReason);
		notifications.error('Websocket disconnected', 5000);
	});

	edgingSocket.onmessage = (event) => {
		const data = JSON.parse(event.data);
		if (data.type == 'payload' && data.origin_id == 'sensor') {
			if (timeSync == 0) {
				timeSync = Date.now() - data.payload.timestamp;
			}
			pressureChart.data.datasets[0].data.push({
				x: timeSync + data.payload.timestamp,
				y: data.payload.raw_pres
			});
			pressureChart.data.datasets[1].data.push({
				x: timeSync + data.payload.timestamp,
				y: data.payload.filtered_pres
			});
			pressureChart.data.datasets[2].data.push({
				x: timeSync + data.payload.timestamp,
				y: data.payload.arousal_value
			});
			arousalChart.data.datasets[0].data.push({
				x: timeSync + data.payload.timestamp,
				y: data.payload.arousal_state
			});
		}
	};

	onDestroy(() => edgingSocket.close());

	let timeSync = 0;

	function daisyColor(name: string, opacity: number = 100) {
		const color = getComputedStyle(document.documentElement).getPropertyValue(name);
		return 'hsla(' + color + '/ ' + Math.min(Math.max(Math.round(opacity), 0), 100) + '%)';
	}

	onMount(() => {
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
						// Arousal Value
						backgroundColor: daisyColor('--s', 10),
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
						duration: 20000,
						refresh: 25
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
							color: daisyColor('--nc', 10)
						},
						ticks: { color: daisyColor('--nc') }
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
						min: 900,
						max: 1400,
						grid: { color: daisyColor('--nc', 10) },
						ticks: {
							stepSize: 100,
							color: daisyColor('--nc')
						},
						border: { color: daisyColor('--nc', 10) }
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
							color: daisyColor('--nc')
						},
						grid: {
							drawOnChartArea: false // only want the grid lines for one axis to show up
						},
						border: { color: daisyColor('--nc', 10) }
					}
				}
			}
		});
		arousalChart = new Chart(arousalChartElement, {
			type: 'line',
			data: {
				datasets: [
					{
						borderColor: daisyColor('--pf'),
						fill: false,
						pointRadius: 0,
						stepped: true,
						data: [],
						yAxisID: 'y'
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
						duration: 20000,
						refresh: 25
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
							color: daisyColor('--nc', 10)
						},
						ticks: { color: daisyColor('--nc') }
					},
					y: {
						type: 'category',
						labels: ['Orgasm', 'Edging', 'Aroused', 'Neutral'],
						position: 'left',
						grid: { color: daisyColor('--nc', 10) },
						ticks: {
							color: daisyColor('--nc')
						},
						border: { color: daisyColor('--nc', 10) }
					}
				}
			}
		});
	});
</script>

<div class="card bg-neutral mt-3 mb-1.5 mx-auto w-11/12">
	<div class="relative h-72 md:h-96 w-full p-2">
		<canvas bind:this={pressureChartElement} />
	</div>
</div>
<div class="card bg-neutral my-1.5 mx-auto w-11/12">
	<div class="relative h-32 w-full p-2">
		<canvas bind:this={arousalChartElement} />
	</div>
</div>
