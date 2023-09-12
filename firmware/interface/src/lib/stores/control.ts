import { writable } from 'svelte/store';
import { page } from '$app/stores';

export type ControlState = {
	active: boolean;
	session_type: number;
	value: number;
	vibrate: number;
};

let controlSocket: WebSocket;

let host: string;

let empty = {
	active: false,
	session_type: 0,
	value: 0,
	vibrate: 0
};

function openControlSocket() {
	controlSocket = new WebSocket('ws://' + host + '/ws/control');
	console.log(`trying to connect to: ${controlSocket.url}`);

	controlSocket.onopen = () => {
		console.log(`connection open to: ${controlSocket.url}`);
	};

	controlSocket.onclose = () => {
		console.log(`connection closed to: ${controlSocket.url}`);
	};

	controlSocket.onmessage = (event) => {
		control.set(JSON.parse(event.data));
	};

	controlSocket.onerror = () => {
		console.log(`connection error with: ${controlSocket.url}`);
	};
}

function createStore() {
	const { subscribe, set, update } = writable(empty);
	return {
		subscribe,
		set,
		update,
		setHost: (_host: string) => {
			host = _host;
			console.log('Set Host: ' + host);
			openControlSocket();
		},
		send: () => {
			controlSocket.send(JSON.stringify(control));
		}
	};
}

export const control = createStore();
