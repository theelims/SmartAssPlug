<script lang="ts">
	import type { PageData } from './$types';
	import { openModal, closeModal } from 'svelte-modals';
	import { notifications } from '$lib/components/toasts/notifications';
	import { user } from '$lib/stores/user';
	import { page } from '$app/stores';
	import { control } from '$lib/stores/control';
	import { analytics } from '$lib/stores/analytics';
	import { Chart, registerables } from 'chart.js';
	import zoomPlugin from 'chartjs-plugin-zoom';
	import { onMount, onDestroy } from 'svelte';
	import ConfirmDialog from '$lib/components/ConfirmDialog.svelte';
	import Download from '~icons/tabler/download';
	import Pull from '~icons/tabler/arrow-big-down-line';
	import Upload from '~icons/tabler/file-upload';
	import Cancel from '~icons/tabler/x';
	import { decode } from 'cbor-x/decode';

	export let data: PageData;

	Chart.register(...registerables);
	Chart.register(zoomPlugin);

	let pressureChartElement: HTMLCanvasElement;
	let pressureChart: Chart;

	let lastPressure: number = 1024;
	let lastdpdt: number = 0;

	onDestroy(() => {
		// Something
	});

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
					zoom: {
						pan: {
							enabled: true,
							mode: 'x',
							modifierKey: 'shift'
						},
						limits: {
							x: { max: 'original' }
						},
						zoom: {
							wheel: {
								enabled: true
							},
							drag: {
								enabled: true,
								modifierKey: 'ctrl'
							},
							pinch: {
								enabled: true
							},
							mode: 'x',
							scaleMode: 'y'
						}
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
						type: 'linear',
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
	});

	let files: FileList;

	async function loadDatFile() {
		await loadCBOR(files[0]);
	}

	async function fetchDataLog() {
		try {
			const datFileResponse = await fetch('/rawdata/datalog.dat', {
				method: 'GET',
				headers: {
					Authorization: $page.data.features.security ? 'Bearer ' + $user.bearer_token : 'Basic',
					'Content-Type': 'application/octet-stream'
				}
			});
			const cbordata = await datFileResponse.blob();
			await loadCBOR(cbordata);
			return;
		} catch (error) {
			console.error('Error:', error);
		}
	}

	async function loadCBOR(cbordata: Blob) {
		// Parse CBOR into chart data object
		console.log(cbordata);
		const arr = new Uint8Array(await cbordata.arrayBuffer());
		console.log(arr);
		const data = decode(arr);
		console.log(data);
		const CBORtoJSON = JSON.stringify(data);
		console.log(CBORtoJSON);
	}

	function convertToJSON() {}

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

	function downloadData(data: Blob, filename: string) {
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
	<div class="justify-between flex m-4 flex-wrap gap-6">
		<div class="flex flex-wrap justify-start gap-4">
			<button
				class="btn btn-primary inline-flex items-center"
				on:click={fetchDataLog}
				disabled={$control.active}
			>
				<Pull class="mr-2 h-5 w-5" /><span>Load last session</span>
			</button>
			<input
				type="file"
				id="binFile"
				class="file-input file-input-bordered file-input-primary w-80"
				bind:files
				accept=".dat"
				on:change={loadDatFile}
			/>
		</div>

		<div class="flex flex-wrap justify-end gap-4">
			<a
				class="btn btn-primary inline-flex items-center"
				href={'http://' + $page.url.host + '/rawdata/datalog.dat'}
				download><Download class="mr-2 h-5 w-5" /><span>Export JSON</span></a
			>
			<a
				class="btn btn-primary inline-flex items-center"
				href={'http://' + $page.url.host + '/rawdata/datalog.dat'}
				download><Download class="mr-2 h-5 w-5" /><span>Export CSV</span></a
			>
		</div>
	</div>
</div>
