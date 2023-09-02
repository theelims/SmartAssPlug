<script lang="ts">
	import { slide } from 'svelte/transition';
	import { cubicOut } from 'svelte/easing';
	import SettingsCard from '$lib/components/SettingsCard.svelte';
	import { user } from '$lib/stores/user';
	import { page } from '$app/stores';
	import { notifications } from '$lib/components/toasts/notifications';
	import Spinner from '$lib/components/Spinner.svelte';
	import MQTT from '~icons/tabler/topology-star-3';

	type BrokerSettings = {
		mqtt_data_path: string;
		mqtt_control_path: string;
		unique_id: string;
	};

	let brokerSettings: BrokerSettings;

	let formField: any;

	async function getBrokerSettings() {
		try {
			const response = await fetch('/rest/brokerSettings', {
				method: 'GET',
				headers: {
					Authorization: $page.data.features.security ? 'Bearer ' + $user.bearer_token : 'Basic',
					'Content-Type': 'application/json'
				}
			});
			brokerSettings = await response.json();
		} catch (error) {
			console.error('Error:', error);
		}
		return;
	}

	let formErrors = {
		uid: false,
		datapath: false,
		controlpath: false
	};

	async function postBrokerSettings() {
		try {
			const response = await fetch('/rest/brokerSettings', {
				method: 'POST',
				headers: {
					Authorization: $page.data.features.security ? 'Bearer ' + $user.bearer_token : 'Basic',
					'Content-Type': 'application/json'
				},
				body: JSON.stringify(brokerSettings)
			});
			if (response.status == 200) {
				notifications.success('Broker settings updated.', 3000);
				brokerSettings = await response.json();
			} else {
				notifications.error('User not authorized.', 3000);
			}
		} catch (error) {
			console.error('Error:', error);
		}
		return;
	}

	function handleSubmitBroker() {
		let valid = true;

		// Validate unique ID
		if (brokerSettings.unique_id.length < 3 || brokerSettings.unique_id.length > 32) {
			valid = false;
			formErrors.uid = true;
		} else {
			formErrors.uid = false;
		}

		// Validate MQTT Data Path
		if (brokerSettings.mqtt_data_path.length > 128) {
			valid = false;
			formErrors.datapath = true;
		} else {
			formErrors.datapath = false;
		}

		// Validate MQTT Control Path
		if (brokerSettings.mqtt_control_path.length > 128) {
			valid = false;
			formErrors.controlpath = true;
		} else {
			formErrors.controlpath = false;
		}

		// Submit JSON to REST API
		if (valid) {
			postBrokerSettings();
			//alert('Form Valid');
		}
	}
</script>

<SettingsCard collapsible={false}>
	<MQTT slot="icon" class="lex-shrink-0 mr-2 h-6 w-6 self-end" />
	<span slot="title">MQTT Broker Settings</span>
	<div class="w-full overflow-x-auto">
		{#await getBrokerSettings()}
			<Spinner />
		{:then nothing}
			<form
				on:submit|preventDefault={handleSubmitBroker}
				novalidate
				bind:this={formField}
				transition:slide|local={{ duration: 300, easing: cubicOut }}
			>
				<div class="grid w-full grid-cols-1 content-center gap-x-4 px-4">
					<div>
						<label class="label" for="uid">
							<span class="label-text text-md">Unique ID</span>
						</label>
						<input
							type="text"
							class="input input-bordered invalid:border-error w-full invalid:border-2 {formErrors.uid
								? 'border-error border-2'
								: ''}"
							bind:value={brokerSettings.unique_id}
							id="uid"
							min="3"
							max="32"
							required
						/>
						<label class="label" for="uid">
							<span class="label-text-alt text-error {formErrors.uid ? '' : 'hidden'}"
								>Unique ID must be between 3 and 32 characters long</span
							>
						</label>
					</div>
					<div>
						<label class="label" for="datatopic">
							<span class="label-text text-md">Data Topic</span>
						</label>
						<input
							type="text"
							class="input input-bordered invalid:border-error w-full invalid:border-2 {formErrors.datapath
								? 'border-error border-2'
								: ''}"
							bind:value={brokerSettings.mqtt_data_path}
							id="datatopic"
							min="0"
							max="128"
							required
						/>
						<label class="label" for="datatopic">
							<span class="label-text-alt text-error {formErrors.datapath ? '' : 'hidden'}"
								>MQTT topic is limited to 128 characters</span
							>
						</label>
					</div>
					<div>
						<label class="label" for="controltopic">
							<span class="label-text text-md">Control Topic</span>
						</label>
						<input
							type="text"
							class="input input-bordered invalid:border-error w-full invalid:border-2 {formErrors.controlpath
								? 'border-error border-2'
								: ''}"
							bind:value={brokerSettings.mqtt_control_path}
							id="controltopic"
							min="0"
							max="128"
							required
						/>
						<label class="label" for="controltopic">
							<span class="label-text-alt text-error {formErrors.controlpath ? '' : 'hidden'}"
								>MQTT topic is limited to 128 characters</span
							>
						</label>
					</div>
				</div>
				<div class="divider mb-2 mt-0" />
				<div class="mx-4 flex flex-wrap justify-end gap-2">
					<button class="btn btn-primary" type="submit">Apply Settings</button>
				</div>
			</form>
		{/await}
	</div>
</SettingsCard>
