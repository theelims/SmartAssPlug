<script lang="ts">
	import { onMount, onDestroy } from 'svelte';
	import { user } from '$lib/stores/user';
	import { page } from '$app/stores';
	import { notifications } from '$lib/components/toasts/notifications';
	import SettingsCard from '$lib/components/SettingsCard.svelte';
	import Spinner from '$lib/components/Spinner.svelte';
	import Filter from '~icons/tabler/filter-cog';
	import Info from '~icons/tabler/info-circle';
	import Save from '~icons/tabler/device-floppy';
	import Reload from '~icons/tabler/reload';

	type FilterSettings = {
		measurement_error: number;
		estimate_error: number;
		process_noise: number;
		interpolate_glitches: boolean;
	};

	let filterSettings: FilterSettings = {
		measurement_error: 1.0,
		estimate_error: 1.0,
		process_noise: 1.0,
		interpolate_glitches: true
	};

	let formField: any;

	async function getFilterSettings() {
		try {
			const response = await fetch('/rest/filterSettings', {
				method: 'GET',
				headers: {
					Authorization: $page.data.features.security ? 'Bearer ' + $user.bearer_token : 'Basic',
					'Content-Type': 'application/json'
				}
			});
			const filter = await response.json();
			filterSettings = filter;
		} catch (error) {
			console.error('Error:', error);
		}
		return;
	}

	async function postFilterSettings(data: FilterSettings) {
		try {
			const response = await fetch('/rest/filterSettings', {
				method: 'POST',
				headers: {
					Authorization: $page.data.features.security ? 'Bearer ' + $user.bearer_token : 'Basic',
					'Content-Type': 'application/json'
				},
				body: JSON.stringify(data)
			});
			if (response.status == 200) {
				notifications.success('Filter settings updated.', 3000);
				const filter = await response.json();
				filterSettings = filter;
			} else {
				notifications.error('User not authorized.', 3000);
			}
		} catch (error) {
			console.error('Error:', error);
		}
	}

	function handleSubmitFilter() {
		let valid = true;

		// Submit JSON to REST API
		if (valid) {
			postFilterSettings(filterSettings);
		}
	}
</script>

<SettingsCard collapsible={false}>
	<Filter slot="icon" class="lex-shrink-0 mr-2 h-6 w-6 self-end" />
	<span slot="title">1D-Kalman Filter Settings</span>
	{#await getFilterSettings()}
		<Spinner />
	{:then nothing}
		<form on:submit|preventDefault={handleSubmitFilter} novalidate bind:this={formField}>
			<div class="grid w-full grid-cols-1 content-center gap-x-4 px-4 sm:grid-cols-2">
				<div>
					<label class="label" for="meaErr">
						<span class="label-text text-md">Measurement Error</span>
					</label>
					<input
						type="number"
						class="input input-bordered invalid:border-error w-full invalid:border-2"
						bind:value={filterSettings.measurement_error}
						id="meaErr"
						min="0"
						max="100"
						step="0.1"
						required
					/>
				</div>
				<div>
					<label class="label" for="estErr">
						<span class="label-text text-md">Estimate Error</span>
					</label>
					<input
						type="number"
						class="input input-bordered invalid:border-error w-full invalid:border-2"
						bind:value={filterSettings.estimate_error}
						id="estErr"
						min="0"
						max="100"
						step="0.1"
						required
					/>
				</div>
				<div>
					<label class="label" for="procNoise">
						<span class="label-text text-md">Process Noise</span>
					</label>
					<input
						type="number"
						class="input input-bordered invalid:border-error w-full invalid:border-2"
						bind:value={filterSettings.process_noise}
						id="procNoise"
						min="0"
						max="100"
						step="0.1"
						required
					/>
				</div>
				<label class="label inline-flex cursor-pointer content-end justify-start gap-4">
					<input
						type="checkbox"
						bind:checked={filterSettings.interpolate_glitches}
						class="checkbox checkbox-primary sm:-mb-9"
					/>
					<span class="sm:-mb-9">Interpolate Glitches</span>
				</label>
			</div>
			<div class="divider my-2" />
			<div class="mx-4 flex flex-wrap justify-end gap-2">
				<button class="btn btn-primary" type="submit">Apply Settings</button>
			</div>
		</form>
	{/await}
</SettingsCard>
